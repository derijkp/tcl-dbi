package require interface

interface::doc dbi_admin-0.1 {
	description {
	}
	subcommands {
	}
}

proc ::interfaces::dbi_admin-0.1 {option args} {
	interface::implement dbi_admin 0.1 [file join $::dbi::dir doc xml interface_dbi.n.xml] {
		-testdb testdbi
	} $option $args
	
	# test interface command
	# ----------------------
	
	interface::test {interface match} {
		$object interface dbi_admin
	} {0.1}
	
	interface::test {interface error} {
		$object interface test12134
	} "$object does not support interface test12134" 1
	
	interface::test {interface list} {
		set list [$object interface]
		set pos [lsearch $list dbi_admin]
		lrange $list $pos [expr {$pos+1}]
	} {dbi_admin 0.1}
	
	# -------------------------------------------------------
	# 							Tests
	# -------------------------------------------------------
	
	interface::test {create} {
		catch {$object close}
		catch {
			$object open $opt(-testdb)
			$object drop
		}
		$object create $opt(-testdb)
		$object open $opt(-testdb)
		$object tables
	} {}
	
	interface::test {create and drop} {
		catch {$object close}
		catch {
			$object open $opt(-testdb)
			$object drop
		}
		$object create $opt(-testdb)
		$object open $opt(-testdb)
		$object drop
		$object exec { }
	} {dbi object has no open database, open a connection first} 1
	
	interface::test {create again after drop} {
		catch {$object close}
		$object create $opt(-testdb)
		$object open $opt(-testdb)
		$object tables
	} {}
	
	$object close
}
