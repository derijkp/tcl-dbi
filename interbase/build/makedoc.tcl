#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

cd [file dir [info script]]
cd ..

foreach file [glob -nocomplain docs/xml/*.xml] {
	set destfile [file join docs man [file tail [file root $file]]]
	if {![file exists $destfile] || ([file mtime $file] > [file mtime $destfile])} {
		puts "creating $destfile"
		if {[catch {exec tmml-man.tcl $file > $destfile} result]} {
			puts error:$result
		}
	}
}
