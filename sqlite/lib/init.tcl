#
# Load libs
# ---------
#

package require dbi

namespace eval dbi::sqlite {}

# $Format: "set ::dbi::sqlite::version 0.8$ProjectMajorVersion$"$
set ::dbi::sqlite::version 0.8
# $Format: "set ::dbi::sqlite::patchlevel $ProjectMinorVersion$"$
set ::dbi::sqlite::patchlevel 9
package provide dbi_sqlite $::dbi::sqlite::version

source $dbi::sqlite::dir/lib/package.tcl
package::init $dbi::sqlite::dir dbi_sqlite

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

set ::dbi::sqlite::privatedbnum 1
proc ::dbi::sqlite::privatedb {db} {
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
	set privatedb($parent) ::dbi::sqlite::priv_$privatedbnum
	incr privatedbnum
	$parent clone $privatedb($parent)
	return $privatedb($parent)
}

array set ::dbi::sqlite::typetrans {
	integer {integer 4}
	double {double 4}
	float {float 4}
}

proc ::dbi::sqlite::info {db args} {
	upvar #0 ::dbi::sqlite::typetrans typetrans
	set db [privatedb $db]
	set len [llength $args]
	if {$len < 1} {error "wrong # args: should be \"$db info option ...\""}
	set type [lindex $args 0]
	switch -exact $type {
		fields {
			if {$len == 2} {
				set table [lindex $args 1]
				return [::dbi::sqlite::fieldsinfo $db $table]
			} else {
				error "wrong # args: should be \"$db info fields ?table?\""
			}
		}
		tables {
			set result [$db exec -flat {SELECT name FROM sqlite_master WHERE type = 'table' and name not like '_dbi_%' ORDER BY name}]
		}
		systemtables {
			set result sqlite_master
			foreach line [$db exec {SELECT name FROM sqlite_master WHERE type = 'table' and name like '_dbi_%' ORDER BY name}] {
				lappend result [lindex $line 0]
			}
		}
		views {
			set result ""
			foreach line [$db exec {SELECT name FROM sqlite_master WHERE type = 'view' and name not like '_dbi_%' ORDER BY name}] {
				lappend result [lindex $line 0]
			}
		}
		table {
			if {$len == 2} {
				set table [lindex $args 1]
				set fields {}
				set result ""
				foreach {pos name type nullable default temp} [$db exec -flat "pragma table_info(\"$table\")"] {
					lappend fields $name
					if {[regexp {^(.*)\(([0-9]+)\)$} $type temp type size]} {
						lappend result type,$name $type
						lappend result length,$name $size
					} elseif {[::info exists typetrans($type)]} {
						foreach {type size} $typetrans($type) break
						lappend result type,$name $type
						lappend result length,$name $size
					} else {
						lappend result type,$name $type
					}
					if {$nullable} {
						lappend result notnull,$name 1
					}
					if {![string equal $default {}]} {
						lappend result default,$name $default
					}
				}
				lappend result fields $fields
				foreach {seq name unique} [$db exec -flat "pragma index_list(\"$table\")"] {
					set columns {}
					foreach {seqno cid cname} [$db exec -flat "pragma index_info('$name')"] {
						lappend columns $cname
					}
					if {$unique} {lappend result unique,$columns $name}
				}
				set code [lindex [$db exec -flat [subst {select "sql" from "sqlite_master" where name = '$table'}]] 0]
				if {[regexp {primary key *\(([^)]+)\)} $code temp columns]} {
					set pk {}
					foreach column [split $columns ,] {
						regexp {^"(.*)"$} $column temp column
						lappend pk $column
					}
					lappend result "primary,$pk" 1
				} else {
					set temp [split $code \n]
					set pos [lsearch -regexp $temp "primary key"]
					set line [lindex $temp $pos]
					set column [lindex $line 0]
					lappend result "primary,$column" 1
				}
				foreach {temp temp field reftable reffield} [$db exec -flat "pragma foreign_key_list(\"$table\")"] {
					foreach key {field reftable reffield} {
						set $key [string trimright [string trimleft [set $key] \"] \"]
					}
					lappend result "foreign,$field" [list $reftable $reffield]
				}
			} else {
				error "wrong # args: should be \"$db info table tablename\""
			}
		}
		default {
			error "error: info about \"$type\" not supported: should be one of: fields, tables, systemtables, views, table"
		}
	}
	return $result
}

proc ::dbi::sqlite::fieldsinfo {db table} {
	set db [privatedb $db]
	if {![llength [$db exec [subst {select name from sqlite_master where name = '$table'}]]]} {
		return -code error "table \"$table\" does not exist"
	}
	set result {}
	foreach line [$db exec "pragma table_info(\"$table\")"] {
		lappend result [lindex $line 1]
	}
	return $result
}

proc ::dbi::sqlite::serial_basic {db table field} {
	if {![llength [$db exec [subst {select name from sqlite_master where name = '$table'}]]]} {
		return -code error "table \"$table\" does not exist"
	}
	if {[lsearch [$db fields $table] $field] == -1} {
		return -code error "field \"$field\" not found in table \"$table\""
	}
	if {![llength [$db exec {select name from sqlite_master where name = '_dbi_serials'}]]} {
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

proc ::dbi::sqlite::serial_shared {db table field} {
	upvar #0 ::dbi::sqlite::shared shared
	if {![::info exists shared($table,$field)]} {
		set temp [$db exec -flat {select sharedtable,sharedfield from _dbi_serials where stable = ? and sfield = ?} $table $field]
		if ![llength $temp] {
			return -code error "no serial on field \"$field\" in table \"$table\""
		}
		if {$temp eq "{} {}"} {
			set shared($table,$field) [list $table $field]
		} else {
			set shared($table,$field) $temp
		}
	}
	return $shared($table,$field)
}

proc ::dbi::sqlite::serial_add_trigger {db table field sharedtable sharedfield} {
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

proc ::dbi::sqlite::serial_add {db table field args} {
	set db [privatedb $db]
	::dbi::sqlite::serial_basic $db $table $field
	if [llength $args] {set current [lindex $args 0]} else {set current 0}
	set serial [$db exec -flat {select "serial" from _dbi_serials where "stable" = ? and "sfield" = ?} $table $field]
	if {[llength $serial]} {
		return -code error "serial already exists on field \"$field\" in table \"$table\""
	}
	$db exec {insert into _dbi_serials (stable,sfield,serial) values (?,?,?)} $table $field $current
	::dbi::sqlite::serial_add_trigger $db $table $field $table $field
}

proc ::dbi::sqlite::serial_share {db table field stable sfield} {
	upvar #0 ::dbi::sqlite::shared shared
	set db [privatedb $db]
	serial_basic $db $table $field
	$db exec {insert into _dbi_serials (stable,sfield,serial,sharedtable,sharedfield) values (?,?,?,?,?)} $table $field -1 $stable $sfield
	::dbi::sqlite::serial_add_trigger $db $table $field $stable $sfield
}

proc ::dbi::sqlite::serial_delete {db table field} {
	set db [privatedb $db]
	serial_basic $db $table $field
	$db exec {delete from _dbi_serials where stable = ? and sfield = ?} $table $field
	set name _dbi_trigger_${table}_${field}
	catch {$db exec [subst {drop trigger $name}]}
}

proc ::dbi::sqlite::serial_set {db table field args} {
	set db [privatedb $db]
	foreach {stable sfield} [::dbi::sqlite::serial_shared $db $table $field] break
	if [llength $args] {
		$db exec [subst {update _dbi_serials set serial = ? where stable = ? and sfield = ?}] [lindex $args 0] $stable $field
	} else {
		return [$db exec {select serial from _dbi_serials where stable = ? and sfield = ?} $stable $field]
	}
}

proc ::dbi::sqlite::serial_next {db table field} {
	set db [privatedb $db]
	foreach {stable sfield} [::dbi::sqlite::serial_shared $db $table $field] break
	$db begin
	$db exec [subst {update _dbi_serials set serial = serial+1 where stable = ? and sfield = ?}] $stable $field
	set current [$db exec {select serial from _dbi_serials where stable = ? and sfield = ?} $stable $field]
	$db commit
	return $current
}

proc ::dbi::sqlite::open_test {db file} {
	if {[string equal $file :memory:]} {return $file}
	if {![file exists $file]} {
		return -code error "file \"$file\" is not a valid database"
	}
	if {![string equal [file pathtype $file] absolute]} {
		set file [file join [pwd] $file]
	}
	return $file
}














