#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

set interface dbi
# $Format: "set version $ProjectMajorVersion$.$ProjectMinorVersion$"$
set version 1.0

package require interface
package require dbi
package require dbi_mysql

set object [dbi_mysql]
set object2 [dbi_mysql]

#interface test dbi_admin-$version $object \
#	-testdb test
#
#interface::test {destroy without opening a database} {
#	dbi_mysql	db
#	db destroy
#	set a 1
#} 1

array set opt [subst {
	-testdb test
	-openargs {-user test}
	-user2 PDR
	-object2 $object2
}]

# $Format: "eval interface test dbi-$ProjectMajorVersion$.$ProjectMinorVersion$ $object [array get opt]"$
eval interface test dbi-1.0 $object [array get opt]

::dbi::opendb
::dbi::initdb

$object destroy
$object2 destroy

interface testsummarize
