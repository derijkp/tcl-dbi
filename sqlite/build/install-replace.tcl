#!/bin/sh
# the next line restarts using tclsh \
exec tclsh8.0 "$0" "$@"

foreach {src dst} $argv break
set f [open $src]
set c [read $f]
close $f
foreach {pattern replacement} [lrange $argv 2 end] {
	regsub -all $pattern $c $replacement c
}
set f [open $dst w]
puts $f $c
close $f
