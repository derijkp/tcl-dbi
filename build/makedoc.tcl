#!/bin/sh
# the next line restarts using tclsh \
exec tclsh8.0 "$0" "$@"

cd [file dir [info script]]
package require Extral
Extral::makedoc [lsort [glob ../lib/*.tcl]] ../docs Extral {
	listcommands lmath stringcommands arraycommands cmd validatecommands
	filing infocommands
	time atexit tempfile structl struct
	convenience ssort
}
