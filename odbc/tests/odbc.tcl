#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

package require dbi
package require dbi_odbc

set object [dbi_odbc]
set object2 [dbi_odbc]
array set opt [subst {
	-columnperm 0
	-testdb testdbi
	-user2 PDR
	-object2 $object2
}]

eval interface test dbi-0.1 $object [array get opt]

dbi::opendb
dbi::initdb

set interface dbi_odbc
set version 0.1

$object destroy
$object2 destroy

interface::testsummarize
