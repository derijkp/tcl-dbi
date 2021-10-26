#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

set testdb localhost:/home/firebird/testdbi.gdb

set f [open ~/.password]
array set config_pw [split [read $f] "\n\t "]
close $f

set interface dbi
set version 1.0

package require interface
package require dbi
package require dbi_firebird

set object [dbi_firebird]
set object2 [dbi_firebird]

#interface test dbi_admin-$version $object \
#	-testdb /home/firebird/testdbi.gdb
# cs
interface test dbi_admin-$version $object \
	-testdb $testdb \
	-openargs [list -user test -password $config_pw(test)]

interface::test {destroy without opening a database} {
	dbi_firebird	db
	db destroy
	set a 1
} 1

#array set opt [subst {
#	-testdb /home/firebird/testdbi.gdb
#	-user2 PDR
#	-object2 $object2
#}]
# cs
array set opt [subst {
	-testdb $testdb
	-openargs {-user test -password $config_pw(test)}
	-user2 test2
	-object2 $object2
}]

eval interface test dbi-$version $object [array get opt]

::dbi::opendb
::dbi::initdb

interface::test {foreign key} {
	$object exec {
		insert into "location" ("type", "inhabitant", "address") 
		values ('home',10000,1)
	}
} {^violation of FOREIGN KEY constraint ".*" on field "inhabitant" in table "location".*} error regexp

interface::test {primary key} {
	$object exec {
		insert into "person" ("id", "first_name") 
		values ('pdr','peter')
	}
} {^violation of PRIMARY or UNIQUE KEY constraint ".*" on field "id" in table "person".*} error regexp

interface::test {interface match} {
	$object supports
} {lines 1 backfetch 1 serials 1 sharedserials 1 blobparams 1 blobids 1 transactions 1 sharedtransactions 1 foreignkeys 1 checks 1 views 1 columnperm 1 roles 1 domains 1 permissions 1}

interface::test {transactions via exec} {
	$object exec {delete from "location";}
	$object exec {delete from "person";}
	set r1 [$object exec {select "first_name" from "person" order by "id";}]
	$object exec {set transaction}
	$object exec {
		insert into "person" values(1,'Peter','De Rijk',20);
	}
	$object exec {
		insert into "person" values(2,'John','Doe',17.5);
		insert into "person" ("id","first_name") values(3,'Jane');
	}
	set r2 [$object exec {select "first_name" from "person" order by "id";}]
	$object exec rollback
	set r3 [$object exec {select "first_name" from "person" order by "id";}]
	$object exec {set transaction}
	$object exec {
		insert into "person" values(1,'Peter','De Rijk',20);
	}
	$object exec {
		insert into "person" values(2,'John','Doe',17.5);
		insert into "person" ("id","first_name") values(3,'Jane');
	}
	$object exec commit
	set r4 [$object exec {select "first_name" from "person" order by "id";}]
	list $r1 $r2 $r3 $r4
} {{} {Peter John Jane} {} {Peter John Jane}}

interface::test {trigger with declare} {
	catch {$object exec {drop trigger person_Update}}
	catch {$object exec {delete from "person"}}
	$object exec {
		create trigger person_Update for "person" before update as
		declare variable current_project varchar(8);
		begin
			update "person"
			set "name" = NEW."name"
			where "id" = OLD."id";
		end
	}
	$object exec {update "person" set "first_name" = 'test' where "id" = 3}
} {}

interface::test {blobid} {
	catch {$object exec {delete from bl;}}
	catch {$object exec {drop table bl;}}
	$object exec {
		create table bl (
			"id" integer,
			"data" blob
		)
	}
	$object exec {insert into bl values (?,?)} 1 abcdefg
	$object exec {insert into bl values (?,?)} 2 123456789
	set result [$object exec -flat -blobid {select "data" from bl}]
	puts blob:[lindex $result 0]
	$object blob open [lindex $result 0]
	set r1 [$object blob get]
	$object blob open [lindex $result 1]
	set r2 [$object blob get]
	list $r1 $r2
} {abcdefg 123456789}

interface::test {blobid segm_size} {
	catch {$object exec {delete from bl;}}
	catch {$object exec {drop table bl;}}
	$object exec {
		create table bl (
			"id" integer,
			"data" blob
		)
	}
	set data {}
	for {set i 0} {$i < 8191} {incr i} {
		append data a
	}
	append data b
	$object exec {insert into bl values (?,?)} 1 $data
	set id [$object exec -blobid {select "data" from bl where "id" = 1}]
	$object blob open $id
	set r1 [$object blob get]
	list [string length $r1] [string index $r1 end]
} {8192 b}

interface::test {blobid segm_size+1} {
	catch {$object exec {delete from bl;}}
	catch {$object exec {drop table bl;}}
	$object exec {
		create table bl (
			"id" integer,
			"data" blob
		)
	}
	set data {}
	for {set i 0} {$i < 8191} {incr i} {
		append data a
	}
	append data b
	append data c
	$object exec {insert into bl values (?,?)} 1 $data
	set id [$object exec -blobid {select "data" from bl where "id" = 1}]
	$object blob open $id
	set r1 [$object blob get]
	list [string length $r1] [string range $r1 8190 8192]
} {8193 abc}

interface::test {blob skip and get} {
	catch {$object exec {delete from bl;}}
	catch {$object exec {drop table bl;}}
	$object exec {
		create table bl (
			"id" integer,
			"data" blob
		)
	}
	$object exec {insert into bl values (?,?)} 1 123456789
	set id [$object exec -blobid {select "data" from bl where "id" = 1}]
	$object blob open $id
	$object blob skip 4
	$object blob get 4
} {5678}

interface::test {blob skip and get twice} {
	catch {$object exec {delete from bl;}}
	catch {$object exec {drop table bl;}}
	$object exec {
		create table bl (
			"id" integer,
			"data" blob
		)
	}
	$object exec {insert into bl values (?,?)} 1 123456789
	set id [$object exec -blobid {select "data" from bl where "id" = 1}]
	$object blob open $id
	$object blob skip 4
	$object blob get 4
	$object blob open $id
	$object blob skip 2
	$object blob get 2
} {34}

interface::test {newblob} {
	catch {$object exec {delete from bl;}}
	catch {$object exec {drop table bl;}}
	$object exec {
		create table bl (
			"id" integer,
			"data" blob
		)
	}
	$object begin
	$object newblob create
	set odata {}
	for {set i 0} {$i < 1000} {incr i} {
		$object newblob put [format "-%9.9d" $i]
		append odata [format "-%9.9d" $i]
	}
	set id [$object newblob close]
	$object exec -blobid {insert into bl values (?,?)} 1 $id
	$object commit
	set data [$object exec {select "data" from bl where "id" = 1}]
	list [string equal $odata $data] [string length $data] [string range $data 1000 1009]
} {1 10000 -000000100}

interface::test {info dependencies} {
	catch {$object exec {drop view "vtest"}}
	catch {$object exec {drop table "test"}}
	catch {$object exec {drop exception test_except}}
	$object exec {
		create table "test" (
			"id" integer not null primary key,
			"person" char(6) references "person"("id") ,
			"score" float check ("score" < 20.0),
			"score2" float,
			"cscore" computed by ("score"*10),
			check ("score2" > "score")
		)
	}
	$object exec {
		create index "testindex" on "test"("score")
	}
	$object exec {
		create index "testindex2" on "test"("score","score2")
	}
	$object exec {
		create exception test_except 'test exception'
	}
	$object exec {
		create trigger test_Insert for "test" before insert as
		begin
			NEW."score" = 10.0;
			exception test_except;
		end
	}
	$object exec {
		create view "vtest" as
		select "score" from "test"
	}
	set result [$object info dependencies test]
	lrange [lsort $result] 0 9
} {{score computed_field *} {score trigger CHECK_*} {score trigger CHECK_*} {score trigger CHECK_*} {score trigger CHECK_*} {score trigger TEST_INSERT} {score view vtest} {score2 trigger CHECK_*} {score2 trigger CHECK_*}} match

interface::test {info dependencies} {
	catch {$object exec {drop domain "testdomain"}} msg
	$object exec {create domain "testdomain" varchar(10)}
	$object info domain testdomain
} {varchar(10) nullable}

interface::test {DECIMAL type} {
	catch {$object exec {drop view "vtest"}}
	catch {$object exec {drop table "test"}}
	catch {$object exec {drop exception test_except}}
	$object exec {
		create table "test" (
			"id" integer not null primary key,
			"dec" decimal(9,4),
			"dec2" decimal(18,4)
		)
	}
	$object exec {
		insert into "test"("id","dec","dec2") values(1,1.5,2.89)
	}
	$object exec {
		insert into "test"("id","dec","dec2") values(2,1.34543,1234.5678)
	}
	$object exec {
		insert into "test"("id","dec","dec2") values(3,?,?)
	} 18.9 1.58
	$object exec {select * from "test"}
} {{1 1.5 2.89} {2 1.3454 1234.5678} {3 18.8999 1.58}}

$object destroy
$object2 destroy

interface testsummarize
