#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

namespace eval interface {}
set type interbase
set testdb /home/ib/testdbi.gdb
set type postgresql
set testdb testdbi
set type odbc
set testdb pgtestdbi

set interface dbi/try
set version 0.1
set interface::testleak 0
set name dbi-01
package require interface
package require dbi
package require dbi_$type

puts "create dbi_$type"
array set opt {
	-testdb /home/ib/testdbi.gdb
	-user2 PDR
	-lines 1
	-columnperm 1
}

set object [dbi_$type]
set object2 [dbi_$type]
set opt(-object2) $object2
set opt(-testdb) $testdb
proc ::dbi::cleandb {} {
	upvar object object
	upvar opt opt
	$object rollback
	set fresult ""
	catch {$object serial delete test id} result
	append fresult $result\n
	catch {$object serial delete address id} result
	append fresult $result\n
	catch {$object exec {drop view "v_test"}} result
	append fresult $result\n
	catch {$object exec {drop table "duse"}} result
	append fresult $result\n
	catch {$object exec {drop table "use"}} result
	append fresult $result\n
	catch {$object exec {drop table "test"}} result
	append fresult $result\n
	catch {$object exec {drop table "types"}} result
	append fresult $result\n
	catch {$object exec {drop table "location"}} result
	append fresult $result\n
	catch {$object exec {drop table "address"}} result
	append fresult $result\n
	catch {$object exec {drop table "person"}} result
	append fresult $result\n
	return $fresult
}
proc ::dbi::createdb {} {
	upvar object object
	upvar opt opt
	$object exec {
		create table "person" (
			"id" char(6) not null primary key,
			"first_name" varchar(100),
			"name" varchar(100),
			"score" double precision
		);
		create table "address" (
			"id" int not null primary key,
			"street" varchar(100),
			"number" varchar(20),
			"code" varchar(10),
			"city" varchar(100)
		);
		create table "location" (
			"type" varchar(100) check ("type" in ('home','work','leisure')),
			"inhabitant" char(6) references "person"("id"),
			"address" integer references "address"("id")
		);
	}
	$object exec {
		create table "use" (
			"id" integer not null primary key,
			"person" integer not null unique references "person"("id"),
			"place" varchar(100),
			"usetime" timestamp,
			"score" float check ("score" < 20.0),
			"score2" float check ("score" < 20.0),
			check ("score2" > "score")
		);
	}
	$object exec {select "id" from "use"}
	catch {$object exec {
		create index "use_score_idx" on "use"("score")
	}}
	$object exec {select "id" from "use"}
	$object exec {
		create table "types" (
			"i" integer,
			"si" smallint,
			"vc" varchar(10),
			"c" char(10),
			"f" float,
			"d" double precision,
			"da" date,
			"t" time,
			"ts" timestamp
		);
	}
	$object exec {create view "v_test" as select "id", "first_name", "name" from "person"}
	catch {$object serial add address id}
}
proc ::dbi::filldb {} {
	upvar object object
	upvar opt opt
	$object exec {
		insert into "person" ("id","first_name","name","score")
			values ('pdr','Peter', 'De Rijk',20);
		insert into "person" ("id","first_name","name","score")
			values ('jd','John', 'Do',17.5);
		insert into "person" ("id","first_name")
			values ('o','Oog');
		insert into "address" ("id","street", "number", "code", "city")
			values (1,'Universiteitsplein', '1', '2610', 'Wilrijk');
		insert into "address" ("id","street", "number", "code", "city")
			values (2,'Melkweg', '10', '1', 'Heelal');
		insert into "address" ("id","street", "number", "code")
			values (3,'Road', '0', '???');
		insert into "location" ("type", "inhabitant", "address")
			select 'work', "person"."id", "address"."id" from "person", "address"
			where "name" = 'De Rijk' and "street" = 'Universiteitsplein';
		insert into "location" ("type", "inhabitant", "address")
			select 'home', "person"."id", "address"."id" from "person", "address"
			where "name" = 'De Rijk' and "street" = 'Melkweg';
		insert into "location" ("type", "inhabitant", "address")
			select 'home', "person"."id", "address"."id" from "person", "address"
			where "name" = 'Do' and "street" = 'Road';
	}
	catch {$object serial set address id 4}
}
proc ::dbi::initdb {} {
	upvar object object
	upvar opt opt
	::dbi::cleandb
	::dbi::createdb
	::dbi::filldb
}
proc ::dbi::opendb {} {
	upvar object object
	upvar opt opt
	$object open $opt(-testdb)
}
#::dbi::opendb
#::dbi::initdb

puts "start test"
	$object open $opt(-testdb)
	$object close
puts ok2
	$object open $opt(-testdb)
	$object close
puts ok3
	$object open $opt(-testdb)
puts close
	$object close
puts ok

#puts "tests done"
$object close
interface::testsummarize

