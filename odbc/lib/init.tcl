#
# Load libs
# ---------
#

package require dbi

namespace eval dbi::odbc {}

# $Format: "set ::dbi::odbc::version 0.$ProjectMajorVersion$"$
set ::dbi::odbc::version 0.0
# $Format: "set ::dbi::odbc::patchlevel $ProjectMinorVersion$"$
set ::dbi::odbc::patchlevel 8
package provide dbi_odbc $::dbi::odbc::version

proc ::dbi::odbc::init {name testcmd} {
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
	#
	# Try to find the compiled library in several places
	#
	if {"[info commands $testcmd]" != "$testcmd"} {
		set libbase {@LIB_LIBRARY@}
		if [regexp ^@ $libbase] {
			if {"$tcl_platform(platform)" == "windows"} {
				regsub {\.} $version {} temp
				set libbase $name$temp[info sharedlibextension]
			} else {
				set libbase lib${name}$version[info sharedlibextension]
			}
		}
		foreach libfile [list \
			[file join $dir build $libbase] \
			[file join $dir .. $libbase] \
			[file join {@LIBDIR@} $libbase] \
			[file join {@BINDIR@} $libbase] \
			[file join $dir $libbase] \
		] {
			if [file exists $libfile] {break}
		}
		#
		# Load the shared library if present
		# If not, Tcl code will be loaded when necessary
		#
		if [file exists $libfile] {
			if {"[info commands $testcmd]" == ""} {
				load $libfile
			}
		} else {
			set noc 1
			source [file join ${dir} lib listnoc.tcl]
		}
		catch {unset libbase}
	}
}
::dbi::odbc::init dbi_odbc dbi_odbc
rename ::dbi::odbc::init {}

lappend auto_path [file join $::dbi::odbc::dir lib]
