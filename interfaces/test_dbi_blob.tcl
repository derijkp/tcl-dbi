# dbi/blob 0.1 interface testing
# ------------------------------

set ::interface::name $interface-$version
::interface::options $::interface::name {
	testdb -testdb testdbi
}

# test interface command
# ----------------------

interface::test {interface match} {
	$object interface dbi/blob
} {0.1}

interface::test {interface error} {
	$object interface test
} "$::interface::object does not support interface test" 1

interface::test {interface list} {
	set list [$object interface]
	set pos [lsearch $list dbi/blob]
	lrange $list $pos [expr {$pos+1}]
} {dbi/blob 0.1}

proc ::interface::initdb {} {
	variable object
	variable testdb
	$object open $testdb
	catch {$object exec {drop table "types"}} result
	$object exec {
		create table "types" (
			"i" integer,
			"si" smallint,
			"vc" varchar(10),
			"c" char(10),
			"f" float,
			"d" double precision,
			"da" date,
			"t" time,
			"ts" timestamp,
			"b" blob
		);
	}
}

# -------------------------------------------------------
# 							Tests
# -------------------------------------------------------
::interface::initdb

interface::test {types including blob} {
	set error ""
	foreach {field value} [list i 20 si 10 vc test c test f 18.5 d 19.5 da "2000-11-18" t 10:40:30.000 ts "2000-11-18 10:40:30.000"] {
		# puts [list $field $value]
		$object exec {delete from "types"}
		$object exec "insert into \"types\"(\"$field\") values(?)" $value
		set rvalue [lindex [lindex [$object exec "select \"$field\" from \"types\" where \"$field\" = ?" $value] 0] 0]
		if {"$value" != "$rvalue"} {
			append error "different values \"$value\" and \"$rvalue\" for $field\n"
		}
		# puts [list $field $rvalue]
	}
	set value {};for {set i 1} {$i < 200} {incr i} {lappend value $i}
	set field b
	# puts [list $field $value]
	$object exec {delete from "types"}
	$object exec "insert into \"types\"(\"$field\") values(?)" $value
	set rvalue [lindex [lindex [$object exec "select \"$field\" from \"types\""] 0] 0]
	if {"$value" != "$rvalue"} {
		append error "different values \"$value\" and \"$rvalue\" for $field\n"
	}
	# puts [list $field $rvalue]
	if [string length $error] {error $error}
	set a 1
} 1

$object close
