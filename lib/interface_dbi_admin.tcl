package require interface

# $Format: "proc ::interfaces::dbi_admin-0.$ProjectMajorVersion$ {option args} {"$
proc ::interfaces::dbi_admin-0.8 {option args} {
	interface::implement dbi_admin $::dbi::version [file join $::dbi::dir doc xml interface_dbi.n.xml] {
		-testdb testdbi
		-openargs {}
	} $option $args
	if {![::info exists opt(-openargs)]} {set opt(-openargs) {}}
	
	# test interface command
	# ----------------------
	
	interface::test {interface match} {
		$object interface dbi_admin
	} $::dbi::version
	
	interface::test {interface error} {
		$object interface test12134
	} "$object does not support interface test12134" error
	
	interface::test {interface list} {
		set list [$object interface]
		set pos [lsearch $list dbi_admin]
		lrange $list $pos [expr {$pos+1}]
	} [list dbi_admin $::dbi::version]
	
	# -------------------------------------------------------
	# 							Tests
	# -------------------------------------------------------
	
	interface::test {create} {
		catch {$object close}
		catch {
			eval {$object open $opt(-testdb)} $opt(-openargs)
			$object drop
		}
		eval {$object create $opt(-testdb)} $opt(-openargs)
		eval {$object open $opt(-testdb)} $opt(-openargs)
		$object tables
	} {}
	
	interface::test {create and drop} {
		catch {$object close}
		catch {
			eval {$object open $opt(-testdb)} $opt(-openargs)
			$object drop
		}
		eval {$object create $opt(-testdb)} $opt(-openargs)
		eval {$object open $opt(-testdb)} $opt(-openargs)
		$object drop
		$object exec { }
	} {dbi object has no open database, open a connection first} error
	
	interface::test {create again after drop} {
		catch {$object close}
		eval {$object create $opt(-testdb)} $opt(-openargs)
		eval {$object open $opt(-testdb)} $opt(-openargs)
		$object tables
	} {}
	
	catch {
		$object close
		eval {$object open $opt(-testdb)} $opt(-openargs)
		$object drop
	}
	interface::test {create, drop, create and open with clone present} {
		catch {$object close}
		eval {$object create $opt(-testdb)} $opt(-openargs)
		eval {$object open $opt(-testdb)} $opt(-openargs)
		$object drop
		eval {$object create $opt(-testdb)} $opt(-openargs)
		eval {$object open $opt(-testdb)} $opt(-openargs)
		set clone [$object clone]
		$object exec {create table "test" (i integer, t varchar(100))}
		$clone exec {insert into "test" values (1,'abcdefgh')}
		$object exec {select * from "test"}
		$object drop
		eval {$object create $opt(-testdb)} $opt(-openargs)
		eval {$object open $opt(-testdb)} $opt(-openargs)
		set clone [$object clone]
		$object exec {create table "test" (i integer, t varchar(100))}
		$object exec {insert into "test" values (1,'abcdefgh')}
		$clone exec {select * from "test"}
	} {{1 abcdefgh}}

	$object close
}
