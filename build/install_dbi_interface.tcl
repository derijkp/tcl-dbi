#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

package require pkgtools
cd [pkgtools::startdir]

if {[llength $argv] != 1} {
	error "wrong # of args, format is: install_simple.tcl <installdir>\nwhere <installdir> is the directory in which the package will be installed"
}

# settings
# --------

set libfiles {lib README pkgIndex.tcl init.tcl DESCRIPTION.txt}

# standard
# --------
set version [pkgtools::version]
set version [join $version .]

cd [pkgtools::startdir]/..
set dest [lindex $argv 0]/dbi$version
puts "Creating $dest"
file delete -force $dest
file mkdir $dest
foreach file $libfiles {
	file copy $file $dest
}
