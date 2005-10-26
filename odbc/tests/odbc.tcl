#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

package require interface
package require dbi
package require dbi_odbc

set object [dbi_odbc]
set object2 [dbi_odbc]
array set opt [subst {
	-columnperm 0
	-testdb testdbi
-openargs {-user test -password test}
	-user2 peter
	-object2 $object2
}]

# $Format: "eval interface test dbi-$ProjectMajorVersion$.$ProjectMinorVersion$ $object [array get opt]"$
eval interface test dbi-1.0 $object [array get opt]

dbi::opendb
dbi::initdb

set interface dbi_odbc
set version 0.1

$object destroy
$object2 destroy

interface testsummarize
