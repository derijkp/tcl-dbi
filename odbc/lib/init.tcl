#
# Load libs
# ---------
#

package require dbi

namespace eval dbi::odbc {}

# $Format: "set ::dbi::odbc::version 0.$ProjectMajorVersion$"$
set ::dbi::odbc::version 0.8
# $Format: "set ::dbi::odbc::patchlevel $ProjectMinorVersion$"$
set ::dbi::odbc::patchlevel 9
package provide dbi_odbc $::dbi::odbc::version

source $dbi::odbc::dir/lib/package.tcl
package::init $dbi::odbc::dir dbi_odbc

lappend auto_path [file join $::dbi::odbc::dir lib]

set ::dbi::odbc::privatedbnum 1
proc ::dbi::odbc::privatedb {db} {
	variable privatedb
	variable privatedbnum
	set parent [$db parent]
	if {[::info exists privatedb($parent)]} {
		if {![string equal [::info commands $privatedb($parent)] ""]} {
			return $privatedb($parent)
		} else {
			unset privatedb($parent)
		}
	}
	set privatedb($parent) ::dbi::odbc::priv_$privatedbnum
	incr privatedbnum
	$parent clone $privatedb($parent)
	return $privatedb($parent)
}
