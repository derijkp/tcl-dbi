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

package require dbi
package require dbi_sqlite3
dbi_sqlite3 db
db close
file delete test.db
db create test.db
db open test.db
db exec {create table test(a char(6) not null primary key,b)}
db exec {insert into test(a,b) values(1,2)}
db exec {insert into test(a,b) values(1,2)}
