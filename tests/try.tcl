#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

namespace eval interface {}
set type interbase
set ::interface::testdb /home/ib/testdbi.gdb
set type odbc
set ::interface::testdb pgtestdbi
set ::interface::interface dbi/try
set ::interface::version 0.1
set ::interface::testleak 0
set ::interface::name dbi-01
set ::interface::user2 PDR

package require dbi
package require dbi_$type

set db [dbi_$type]
set db2 [dbi_$type]
set ::interface::object $db
set ::interface::object2 $db2
set object $::interface::object
set object2 $::interface::object2
set user2 $::interface::user2

proc ::interface::cleandb {} {
	variable object
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

proc ::interface::createdb {} {
	variable object
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

proc ::interface::filldb {} {
	variable object
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

proc ::interface::initdb {} {
	cleandb
	createdb
	filldb
}

proc ::interface::opendb {} {
	variable object
	variable testdb
	$object open $testdb
}

interface::opendb
interface::initdb
puts [$object tables]

interface::test {fetch 1} {
	$object exec -usefetch {select * from "person"}
	$object fetch 1
} {jd John Do 17.5}

interface::test {repeat fetch 1} {
	$object exec -usefetch {select * from "person"}
	$object fetch 1
	$object fetch 1
} {jd John Do 17.5}

interface::testsummarize

#catch {unset a}
#set table location
#array set a [$db info table $table]
#parray a
#interface::initdb
#$db exec {update "person" set "name" = ? where "id" = ?} try t
