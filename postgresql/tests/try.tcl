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
set interface dbi
set version 1.0

$object close
::dbi::open
::dbi::cleandb
::dbi::createdb
::dbi::filldb

set field t
set value "2000-11-18 10:40:30.000"
$object exec {delete from "types"}
$object exec "insert into \"types\"(\"$field\") values(?)" $value
set rvalue [lindex [lindex [$object exec "select \"$field\" from \"types\" where \"$field\" = ?" $value] 0] 0]
