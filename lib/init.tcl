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
set ::dbi::version 0.0
# $Format: "set ::dbi::patchlevel $ProjectMinorVersion$"$
set ::dbi::patchlevel 4
package provide dbi $::dbi::version

proc _package_loadlib {name version library args} {
	upvar #0 _package_$name _config
	# correct dirs
	if [file exists [file join $_config(execdir) lib]] {
		set _config(dir) $_config(execdir)
	}
	set ::dbi::dir $_config(dir)
	if [file exists [file join $_config(execdir) bin]] {
		set _config(bindir) [file join $_config(execdir) bin]
	}
	if [file exists [file join $_config(execdir) data]] {
		set _config(datadir) [file join $_config(execdir) data]
	}
	# Use the library given by the config?
	if [file exists $library] {
		::load $libfile
		return $libfile
	}
	#
	# Try to find the compiled library in several places
	#
	global tcl_platform
	if {"$tcl_platform(platform)" == "windows"} {
		regsub {\.} $version {} temp
		lappend libbase $name${temp}g[::info sharedlibextension]
		lappend libbase $name$temp[::info sharedlibextension]
	} else {
		lappend libbase lib${name}${version}g[::info sharedlibextension]
		lappend libbase lib${name}$version[::info sharedlibextension]
	}
	lappend args [file join $_config(dir) build] \
		[file join $_config(dir) ..] \
		[file join $_config(libdir)] \
		[file join $_config(bindir)] \
		[file join $_config(dir)]
	foreach dir $args {
		foreach base $libbase {
			set libfile [file join $dir $base]
			if [file exists $libfile] {break}
		}
		if [file exists $libfile] {break}
	}
	#
	# Load the shared library if present
	# If not, Tcl code will be loaded when necessary
	#
	if [file exists $libfile] {
		::load $libfile
	} else {
		error "library for $name $version not found in dirs [file join $_config(dir) build] \
		[file join $_config(dir) ..] \
		[file join $_config(libdir)] \
		[file join $_config(bindir)] \
		[file join $_config(dir)]"
	}
	return $libfile
}

#
# Try to find the compiled library in several places
#
if {"[::info commands dbi]" != "dbi"} {
	_package_loadlib dbi $::dbi::version  $_package_dbi(library)
}

lappend auto_path [file join $::dbi::dir lib]

proc dbi::info {item} {
	switch $item {
		types {
			# find types
			catch {unset types}
			catch {package require xxx}
			foreach type [package names] {
				if [regexp {^dbi_(.*)} $type temp type] {
					set types($type) {}
				}
			}
			foreach type [dbi info typesloaded] {
				set types($type) {}
			}
			return [lsort [array names types]]
		}
		default {
			error "unknown info item \"$item\", should be one of types, typesloaded"
		}
	}
}

proc dbi::load {type} {
	set typesloaded [dbi info typesloaded]
	if {[lsearch $typesloaded $type] != -1} return
	set types [::dbi::info types]
	if {[lsearch $types $type] == -1} {
		error "Cannot find dbi type \"$type\""
	}
	package require dbi_$type
}

lappend auto_path $dbi::dir
