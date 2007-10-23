#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

set f [open ~/.password]
array set config_pw [split [read $f] "\n\t "]
close $f

package require Extral
package require interface
namespace eval interface {}
set type postgresql
set testdb testdbi
set type sqlite
set testdb test.db
set type odbc
set testdb testdbi
set type mysql
set testdb test
set type firebird
set testdb localhost:/home/ib/testdbi.gdb

package require dbi
package require dbi_$type
set interface::testleak 0
set object [dbi_$type]
set object2 [dbi_$type]
array set opt [subst {
	-columnperm 0
	-testdb $testdb
	-openargs {-user test -password $config_pw(test)}
	-user2 peter
	-object2 $object2
}]

dbi::setup
dbi::opendb
#dbi::initdb
dbi::cleandb
::dbi::createdb

