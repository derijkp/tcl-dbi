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

::dbi::setup
::dbi::opendb
::dbi::cleandb
::dbi::createdb
::dbi::filldb

interface::test {parameters with comments and literals} {
	$object exec {select "id",'?' /* selecting what ? */ from "person" where "name" = ? and "score" = ?} {De Rijk} 19.5
} {{pdr ?}}

