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

proc lconcat args {return $args}
$object function lconcat lconcat

	$object exec {insert into "person" ("id","first_name","name") values ('jd3','John',"Test Case")}
	$object exec {
		select "first_name",list_concat("name")
		from "person" where "first_name" regexp 'ohn' group by "first_name"
	}
