#
# Load libs
# ---------
#

namespace eval dbi::mysql {}

# $Format: "set ::dbi::mysql::version $ProjectMajorVersion$.$ProjectMinorVersion$"$
set ::dbi::mysql::version 1.0
# $Format: "set ::dbi::mysql::patchlevel $ProjectPatchLevel$"$
set ::dbi::mysql::patchlevel 0
package provide dbi_mysql $::dbi::mysql::version

package require pkgtools
pkgtools::init $dbi::mysql::dir dbi_mysql

#
# Procs
# -----
#
proc ::dbi::? {expr truevalue falsevalue} {
	uplevel if [list $expr] {{set ::dbi::temp 1} else {set ::dbi::temp 0}}
	if $::dbi::temp {return $truevalue} else {return $falsevalue}
}

proc ? {expr truevalue falsevalue} {
	uplevel if [list $expr] {{set ::dbi::temp 1} else {set ::dbi::temp 0}}
	if $::dbi::temp {return $truevalue} else {return $falsevalue}
}

proc ::dbi::string_split {string splitstring} {
	set result ""
	set len [string length $splitstring]
	while 1 {
		set pos [string first $splitstring $string]
		if {$pos == -1} {
			lappend result $string
			break
		}
		lappend result [string range $string 0 [expr {$pos-1}]]
		set string [string range $string [expr {$pos+$len}] end]
	}
	return $result
}

set ::dbi::mysql::privatedbnum 1
proc ::dbi::mysql::privatedb {db} {
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
	set privatedb($parent) ::dbi::mysql::priv_$privatedbnum
	incr privatedbnum
	$parent clone $privatedb($parent)
	return $privatedb($parent)
}

array set ::dbi::mysql::typetrans {
	tinyint {integer 1}
	smallint {integer 2}
	mediumint {integer 3}
	int {integer 4}
	bigint {integer 8}
	double {double 4}
	float {float 4}
}

proc ::dbi::mysql::info {db args} {
	upvar #0 ::dbi::mysql::typetrans typetrans
	set db [privatedb $db]
	set len [llength $args]
	if {$len < 1} {error "wrong # args: should be \"$db info option ...\""}
	set type [lindex $args 0]
	switch -exact $type {
		user {
			set result [string toupper [$db exec {select user from rdb$database}]]
		}
		fields {
			if {$len == 2} {
				set table [lindex $args 1]
				return [::dbi::mysql::fieldsinfo $db $table]
			} else {
				error "wrong # args: should be \"$db info fields ?table?\""
			}
		}
		databases {
			set result [lsort [$db exec -flat {show databases}]]
		}
		tables {
			set result [lsort [$db exec -flat {show tables}]]
		}
		table {
			if {$len != 2} {
				error "wrong # args: should be \"$db info table tablename\""
			}
			set table [lindex $args 1]
			set fields {}
			set result ""
			if {[catch {$db exec -flat "show columns from $table"} list]} {
				error "table \"$table\" does not exist"
			}
			foreach {name type null key default extra} $list {
				lappend fields $name
				catch {unset size}
				regexp {^(.*)\(([0-9]+)\)$} $type temp type size
				if {[::info exists typetrans($type)]} {
					foreach {type size} $typetrans($type) break
					lappend result type,$name $type
					lappend result length,$name $size
				} elseif {[::info exists size]} {
					lappend result type,$name $type
					lappend result length,$name $size
				} else {
					lappend result type,$name $type
				}
				if {![string equal $null YES]} {
					lappend result notnull,$name 1
				}
				if {![string equal $default NULL]} {
					lappend result default,$name $default
				}
			}
			lappend result fields $fields
			catch {unset ind}
			catch {unset indinfo}
			foreach {
				Table Non_unique Key_name Seq_in_index Column_name Collation Cardinality Sub_part Packed Comment
			} [$db exec -flat "show index from $table"] {
				lappend ind($Key_name) $Column_name
				set indinfo(nonunique,$Key_name) $Non_unique
			}
			foreach index [array names ind] {
				if {[string equal $index PRIMARY]} {
					lappend result primary,$ind($index) 1
				} elseif {$indinfo(nonunique,$index) == 0} {
					lappend result unique,$ind($index) $index
				}
			}
		}
		default {
			error "error: info about $type not supported"
		}
	}
	return $result
}

proc ::dbi::mysql::fieldsinfo {db table} {
	set db [privatedb $db]
	if {[catch {$db exec "show columns from $table"} list]} {
		error "table \"$table\" does not exist"
	}
	set result {}
	foreach line $list {
		lappend result [lindex $line 0]
	}
	return $result
}

proc ::dbi::mysql::serial_basic {db table field} {
	if {![llength [$db exec [subst {select name from mysql_master where name = '$table'}]]]} {
		return -code error "table \"$table\" does not exist"
	}
	if {[lsearch [$db fields $table] $field] == -1} {
		return -code error "field \"$field\" not found in table \"$table\""
	}
	if {![llength [$db exec {select name from mysql_master where name = '_dbi_serials'}]]} {
		# $db exec {drop table _dbi_serials}
		$db exec {
			create table _dbi_serials (
				stable text,
				sfield text,
				serial integer,
				sharedtable text,
				sharedfield text,
				unique(stable,sfield)
			)
		}
	}
}

proc ::dbi::mysql::serial_shared {db table field} {
	upvar #0 ::dbi::mysql::shared shared
	if {![::info exists shared($table,$field)]} {
		set temp [$db exec [subst {select sharedtable,sharedfield from _dbi_serials where stable = '$table' and sfield = '$field'}]]
		if ![llength $temp] {
			return -code error "no serial on field \"$field\" in table \"$table\""
		}
		set shared($table,$field) $temp
	}
	return $shared($table,$field)
}

proc ::dbi::mysql::serial_add_trigger {db table field sharedtable sharedfield} {
	set name _dbi_trigger_${table}_${field}
	set fields [$db fields $table]
	set pos [lsearch $fields $field]
	set fields [lreplace $fields $pos $pos]
	set sql [subst {
		create trigger $name before insert on $table when new."$field" is null
		begin
			update _dbi_serials set serial = serial+1  where stable = '$sharedtable' and sfield = '$sharedfield';
			insert into "$table" ("$field","[join $fields \",\"]")
				values ((select serial from _dbi_serials where stable = '$sharedtable' and sfield = '$sharedfield'),
				new."[join $fields \",new.\"]");
			select case when 1 then raise(ignore) end;
		end;
	}]
	$db exec $sql
}

proc ::dbi::mysql::serial_add {db table field args} {
	set db [privatedb $db]
	::dbi::mysql::serial_basic $db $table $field
	if [llength $args] {set current [lindex $args 0]} else {set current 0}
	set serial [$db exec -flat {select "serial" from _dbi_serials where "stable" = ? and "sfield" = ?} $table $field]
	if {[llength $serial]} {
		return -code error "serial already exists on field \"$field\" in table \"$table\""
	}
	$db exec {insert into _dbi_serials (stable,sfield,serial) values (?,?,?)} $table $field $current
	::dbi::mysql::serial_add_trigger $db $table $field $table $field
}

proc ::dbi::mysql::serial_share {db table field stable sfield} {
	upvar #0 ::dbi::mysql::shared shared
	set db [privatedb $db]
	serial_basic $db $table $field
	$db exec {insert into _dbi_serials (stable,sfield,serial,sharedtable,sharedfield) values (?,?,?,?,?)} $table $field -1 $stable $sfield
	::dbi::mysql::serial_add_trigger $db $table $field $stable $sfield
}

proc ::dbi::mysql::serial_delete {db table field} {
	set db [privatedb $db]
	serial_basic $db $table $field
	$db exec {delete from _dbi_serials where stable = ? and sfield = ?} $table $field
	set name _dbi_trigger_${table}_${field}
	catch {$db exec [subst {drop trigger $name}]}
}

proc ::dbi::mysql::serial_set {db table field args} {
	set db [privatedb $db]
	foreach {stable sfield} [::dbi::mysql::serial_shared $db $table $field] break
	if [llength $args] {
		$db exec [subst {update _dbi_serials set serial = [lindex $args 0] where stable = '$table' and sfield = '$field'}]
	} else {
		return [$db exec {select serial from _dbi_serials where stable = ? and sfield = ?} $table $field]
	}
}

proc ::dbi::mysql::serial_next {db table field} {
	set db [privatedb $db]
	foreach {stable sfield} [::dbi::mysql::serial_shared $db $table $field] break
	$db begin
	$db exec [subst {update _dbi_serials set serial = serial+1 where stable = '$table' and sfield = '$field'}]
	set current [$db exec {select serial from _dbi_serials where stable = ? and sfield = ?} $table $field]
	$db commit
	return $current
}

proc ::dbi::mysql::open_test {db file} {
	if {![file exists $file]} {
		return -code error "file \"$file\" is not a valid database"
	}
	if {![string equal [file pathtype $file] absolute]} {
		set file [file join [pwd] $file]
	}
	return $file
}
