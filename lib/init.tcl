# Initialisation of the Gendb package
#
# Copyright (c) 1998 Peter De Rijk
#
# See the file "README.txt" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
# =============================================================
namespace eval ::dbi {}
# $Format: "set ::dbi::version $ProjectMajorVersion$.$ProjectMinorVersion$"$
set ::dbi::version 1.0
# $Format: "set ::dbi::patchlevel $ProjectPatchLevel$"$
set ::dbi::patchlevel 0
package provide dbi $::dbi::version

# define interfaces
#
namespace eval ::interface {}

lappend auto_path [file join $::dbi::dir lib]
lappend auto_path $dbi::dir
proc dbi::init {} {
	foreach file [glob $::dbi::dir/*/pkgIndex.tcl] {
		set dir [file dir $file]
		source $file
	}
}
dbi::init

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

# in some situations, modules like dbi_interbase hang if an env variable has not been
# accessed. I have not found out why, but until then just protect against it
catch {set env(HOME)}
