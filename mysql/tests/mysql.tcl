#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

set interface dbi
# $Format: "set version 0.$ProjectMajorVersion$"$
set version 0.8

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

# $Format: "eval interface test dbi-0.$ProjectMajorVersion$ $object [array get opt]"$
eval interface test dbi-0.8 $object [array get opt]

::dbi::opendb
::dbi::initdb

$object destroy
$object2 destroy

interface testsummarize
