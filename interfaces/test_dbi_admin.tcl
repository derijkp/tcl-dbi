# dbi/admin 0.1 interface testing
# -------------------------

set ::interface::name $interface-$version
::interface::options $::interface::name {
	testdb -testdb testdbi
}

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
		$object open $testdb
		$object drop
	}
	$object create $testdb
	$object open $testdb
	$object tables
} {}

interface::test {create and drop} {
	variable testdb
	catch {$object close}
	catch {
		$object open $testdb
		$object drop
	}
	$object create $testdb
	$object open $testdb
	$object drop
	$object exec { }
} {dbi object has no open database, open a connection first} 1

interface::test {create again after drop} {
	variable testdb
	catch {$object close}
	$object create $testdb
	$object open $testdb
	$object tables
} {}

$object close

