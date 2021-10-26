#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

package require interface
package require dbi
package require dbi_postgresql

set object [dbi_postgresql]
set object2 [dbi_postgresql]

array set opt [subst {
	-testdb testdbi
	-user2 pdr
	-object2 $object2
}]

# $Format: "eval interface test dbi-$ProjectMajorVersion$.$ProjectMinorVersion$ $object [array get opt]"$
eval interface test dbi-1.0 $object [array get opt]

$object destroy
$object2 destroy

interface testsummarize

$db info table location
