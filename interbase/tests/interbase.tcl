#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

set interface dbi
# $Format: "set version 0.$ProjectMajorVersion$"$
set version 0.8

package require dbi
package require dbi_interbase

set object [dbi_interbase]
set object2 [dbi_interbase]

interface test dbi_admin-$version $object \
	-testdb /home/ib/testdbi.gdb

interface::test {destroy without opening a database} {
	dbi_interbase	db
	db destroy
	set a 1
} 1

array set opt [subst {
	-testdb /home/ib/testdbi.gdb
	-user2 PDR
	-object2 $object2
}]

# $Format: "eval interface test dbi-0.$ProjectMajorVersion$ $object [array get opt]"$
eval interface test dbi-0.8 $object [array get opt]

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
	lsort [$object supports]
} {blobids blobparams checks columnperm domains foreignkeys permissions roles sharedtransactions}

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
	$object newblob create
	set odata {}
	for {set i 0} {$i < 1000} {incr i} {
		$object newblob put [format "-%9.9d" $i]
		append odata [format "-%9.9d" $i]
	}
	set id [$object newblob close]
	$object exec -blobid {insert into bl values (?,?)} 1 $id
	set data [$object exec {select "data" from bl where "id" = 1}]
	list [string equal $odata $data] [string length $data] [string range $data 1000 1009]
} {1 10000 -000000100}

interface::test {serial share} {
	$object exec {delete from "location";}
	$object exec {delete from "address";}
	$object exec {delete from "types"}
	catch {$object serial delete address id}
	catch {$object serial delete types i}
	$object serial add types i
	$object serial share address id types i
	$object exec {insert into "types" ("d") values (20)}
	$object exec {insert into "types" ("d") values (21)}
	$object exec {insert into "address" ("street") values ('test')}
	$object exec {select "id","street" from "address" order by "id"}
} {{3 test}}

$object destroy
$object2 destroy

interface testsummarize
