
# dbi/admin 0.1 interface testing
# -------------------------------

set ::interface::name $interface-$version
if {[info exists ::interface::options(-testdb)]} {
	set ::interface::testdb $::interface::options(-testdb)
} else {
	set ::interface::testdb testdbi
}

# create object
# -------------

namespace eval ::interface {set interface::object [$createobj]}

# test interface command
# ----------------------

interface::test {interface match} {
	$object interface dbi/admin
} {0.1}

interface::test {interface error} {
	$object interface test
} "$::interface::object does not support interface test" 1

interface::test {interface list} {
	set list [$object interface]
	set pos [lsearch $list dbi/admin]
	lrange $list $pos [expr {$pos+1}]
} {dbi/admin 0.1}

# -------------------------------------------------------
# 							Tests
# -------------------------------------------------------

interface::test {create} {
	variable testdb
	catch {$object close}
	catch {
		$object open $testdb -user pdr -password pdr
		$object drop
	}
	$object create $testdb -user pdr -password pdr
	$object open $testdb -user pdr -password pdr
	$object tables
} {}

interface::test {create and drop} {
	variable testdb
	catch {$object close}
	catch {
		$object open $testdb -user pdr -password pdr
		$object drop
	}
	$object create $testdb -user pdr -password pdr
	$object open $testdb -user pdr -password pdr
	$object drop
	$object exec { }
} {dbi object has no open database, open a connection first} 1


interface::testsummarize

