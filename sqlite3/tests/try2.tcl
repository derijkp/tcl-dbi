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
::dbi::createdb
::dbi::filldb

proc test {a b} {return [list $a $b]}
$object function test test
$object exec {select test(1,2)}

proc nocase_compare {a b} {
	return [string compare [string tolower $a] [string tolower $b]]
}
$object collate nocase nocase_compare
$object exec {insert into "person" values('jd2','john','do',18)}
$object exec {select * from "person" order by "first_name"}
$object exec {select * from "person" order by "first_name" collate nocase}

$object exec {select regexp('[pt]est','test')}
$object exec {select "first_name" from "person" where "first_name" regexp 'ohn'}
