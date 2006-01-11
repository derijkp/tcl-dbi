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
::dbi::filldb


interface::test {unset error} {
	catch {$object exec {delete from types}}
	foreach v {a20 a2 a1 a2b a4 a22 a10} {
		$object exec {insert into types(t) values(?)} $v
	}
	$object exec {select t from types order by t collate DICT}
} {a1 a2 a2b a4 a10 a20 a22}

