#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

package require interface
package require dbi
package require dbi_postgresql

set object [dbi_postgresql]
set object2 [dbi_postgresql]
set db $object

array set opt [subst {
	-testdb testdbi
	-user2 pdr
	-object2 $object2
}]

catch {interfaces::dbi-1.0}
dbi::setup

$object close
dbi::open
::dbi::cleandb
::dbi::createdb
::dbi::filldb

 $object exec {select * from "person"}
