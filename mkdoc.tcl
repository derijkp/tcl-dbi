#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

set tmmlman ~/tcl/tmml/tools/tmml-man.tcl
foreach file [glob doc/xml/*.xml] {
	exec $tmmlman $file > [file join doc man [file tail [file root $file]]]
}
