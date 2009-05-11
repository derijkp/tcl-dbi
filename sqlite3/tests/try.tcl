#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

set interface dbi
# $Format: "set version $ProjectMajorVersion$.$ProjectMinorVersion$"$
set version 1.0

package require interface
package require dbi
package require dbi_sqlite3
set object [dbi_sqlite3]
set object2 [dbi_sqlite3]
array set opt [subst {
	-testdb test.db
	-user2 PDR
	-object2 $object2
}]
file delete $opt(-testdb)
source ../../tests/tools.tcl
dbi_sqlite3 db
db create $opt(-testdb)
::dbi::opendb
::dbi::initdb

	catch {$object delete {person test}}
	catch {$object delete {person test2}}
	$object begin
	$object set {person test} first_name first name test score 19.5
	$object set {person test2} first_name first2
	$object rollback
	$object get {person test}

db open :memory:
db exec {
	create table "person" (
		"id" char(6) not null primary key,
		"name" varchar(100)
	)
}
set list {1 a 2 b 3 c 4 d 5 e 6 f 7 g 8 h 9 i 10 j}
db exec {delete from person}
time {
	foreach {id name} $list {
		db exec -cache {
			insert into "person" (id,name) values (?,?)
		} $id $name
	}
}
db exec {select * from person}
db exec {delete from person}
time {
	foreach {id name} $list {
		db exec {
			insert into "person" (id,name) values (?,?)
		} $id $name
	}
}
db exec {select * from person}

proc lconcat args {return $args}
$object function lconcat lconcat

	$object exec {insert into "person" ("id","first_name","name") values ('jd3','John',"Test Case")}
	$object exec {
		select "first_name",list_concat("name")
		from "person" where "first_name" regexp 'ohn' group by "first_name"
	}
