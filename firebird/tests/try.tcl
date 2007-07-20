#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"

# valgrind --leak-check=full --gen-suppressions=yes --suppressions=/home/peter/.valgrind/file.supp tclsh

package require dbi
package require dbi_firebird
package require interface

set f [open ~/.password]
array set config_pw [split [read $f] "\n\t "]
close $f

catch {interface doc dbi}
set interface dbi
set version 1.0
dbi::setup
set object [dbi_firebird db]
set object2 [dbi_firebird]

array set opt [subst {
	-testdb localhost:/home/ib/testdbi.gdb
	-openargs {-user test -password $config_pw(test)}
	-user2 test2
	-object2 $object2
}]

::dbi::opendb
::dbi::cleandb

::dbi::createdb
::dbi::filldb
#dbi::initdb
$object exec {select * from "person"}

interface::test {error when info on closed object} {
	$object close
	set fields [$object info fields person]
} {dbi object has no open database, open a connection first} error
catch {::dbi::opendb}

interface::test {drop table not after -cache} {
	$object exec {create table "t" (i integer)}
	$object exec {insert into "t" values(1)}
	$object exec {select * from "t"}
	$object exec {drop table "t"}
	lsearch [$object tables] t
} -1

