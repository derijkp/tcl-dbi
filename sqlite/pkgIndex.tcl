# Tcl package index file, version 1.0
# This file is sourced either when an application starts up or
# by a "package unknown" script.  It invokes the
# "package ifneeded" command to set up package-related
# information so that packages will be loaded automatically
# in response to "package require" commands.  When this
# script is sourced, the variable $dir must contain the
# full path name of this file's directory.

# $Format: "package ifneeded dbi_sqlite $ProjectMajorVersion$.$ProjectMinorVersion$ \\"$
package ifneeded dbi_sqlite 1.0 \
[subst -nocommands {
	namespace eval ::dbi {}
	namespace eval ::dbi::sqlite {}
	set ::dbi::sqlite::dir [list $dir]
	source [file join [list $dir] lib init.tcl]
}]
