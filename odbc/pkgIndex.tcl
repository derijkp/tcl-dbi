# Tcl package index file, version 1.0
# This file is sourced either when an application starts up or
# by a "package unknown" script.  It invokes the
# "package ifneeded" command to set up package-related
# information so that packages will be loaded automatically
# in response to "package require" commands.  When this
# script is sourced, the variable $dir must contain the
# full path name of this file's directory.

# $Format: "package ifneeded dbi_odbc 0.$ProjectMajorVersion$ \\"$
package ifneeded dbi_odbc 0.0 \
[subst -nocommands {
	namespace eval ::dbi {}
	namespace eval ::dbi::odbc {}
	set ::dbi::odbc::execdir $dir
	if [file exists [file join $dir lib init.tcl]] {
		source [file join $dir lib init.tcl]
	} else {
		source [file join @TCLLIBDIR@ lib init.tcl]
	}
}]
