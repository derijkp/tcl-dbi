# Initialisation of the Gendb package
#
# Copyright (c) 1998 Peter De Rijk
#
# See the file "README.txt" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
# =============================================================
# $Format: "set ::dbi::version 0.$ProjectMajorVersion$"$
set ::dbi::version 0.0
# $Format: "set ::dbi::patchlevel $ProjectMinorVersion$"$
set ::dbi::patchlevel 1
package provide dbi $::dbi::version

# Load the compiled code
if {"[info commands dbi]" == ""} {
	load [file join [set ::dbi::dir] dbi[info sharedlibextension]]
}

lappend auto_path [file join $::dbi::dir lib]
