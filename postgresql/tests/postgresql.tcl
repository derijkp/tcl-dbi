#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

package require dbi
package require dbi_postgresql

set object [dbi_postgresql]
set object2 [dbi_postgresql]

array set opt [subst {
	-testdb testdbi
	-user2 pdr
	-object2 $object2
}]

eval interface test dbi-0.1 $object [array get opt]

$object destroy
$object2 destroy

interface testsummarize

