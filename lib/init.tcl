# Initialisation of the Gendb package
#
# Copyright (c) 1998 Peter De Rijk
#
# See the file "README.txt" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
# =============================================================
namespace eval ::dbi {}
# $Format: "set ::dbi::version 0.$ProjectMajorVersion$"$
set ::dbi::version 0.8
# $Format: "set ::dbi::patchlevel $ProjectMinorVersion$"$
set ::dbi::patchlevel 5
package provide dbi $::dbi::version

proc ::dbi::init {name testcmd} {
	global tcl_platform
	foreach var {version patchlevel execdir dir bindir datadir} {
		variable $var
	}
	#
	# If the following directories are present in the same directory as pkgIndex.tcl, 
	# we can use them otherwise use the value that should be provided by the install
	#
	if [file exists [file join $execdir lib]] {
		set dir $execdir
	} else {
		set dir {@TCLLIBDIR@}
	}
	if [file exists [file join $execdir bin]] {
		set bindir [file join $execdir bin]
	} else {
		set bindir {@BINDIR@}
	}
	if [file exists [file join $execdir data]] {
		set datadir [file join $execdir data]
	} else {
		set datadir {@DATADIR@}
	}
}

dbi::init dbi list_pop
rename dbi::init {}

# define interfaces
#
package require interface

lappend auto_path [file join $::dbi::dir lib]
lappend auto_path $dbi::dir

proc dbi::info {item} {
	switch $item {
		types {
			# find types
			catch {unset types}
			catch {package require xxx}
			foreach type [package names] {
				if [regexp {^dbi_(.*)} $type] {
					set types($type) {}
				}
			}
			return [lsort [array names types]]
		}
		default {
			error "unknown info item \"$item\", should be one of types"
		}
	}
}
