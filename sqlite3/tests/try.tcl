#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

set interface dbi
# $Format: "set version 0.$ProjectMajorVersion$"$
set version 0.8

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


interface::test {get field field} {
	$object get {person pdr} first_name name
} {Peter {De Rijk}}

interface::test {set field} {
	$object set {person pdr} first_name Pe
	$object get {person pdr} first_name
} {Pe}

