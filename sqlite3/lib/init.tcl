#
# Load libs
# ---------
#

namespace eval dbi::sqlite3 {}

# $Format: "set ::dbi::sqlite3::version $ProjectMajorVersion$.$ProjectMinorVersion$"$
set ::dbi::sqlite3::version 1.0
# $Format: "set ::dbi::sqlite3::patchlevel $ProjectPatchLevel$"$
set ::dbi::sqlite3::patchlevel 0

package require pkgtools
pkgtools::init $dbi::sqlite3::dir dbi_sqlite3 dbi_sqlite3 {} Dbi_sqlite3

package provide dbi_sqlite3 $::dbi::sqlite3::version

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

set ::dbi::sqlite3::privatedbnum 1
proc ::dbi::sqlite3::privatedb {db} {
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
	set privatedb($parent) ::dbi::sqlite3::priv_$privatedbnum
	incr privatedbnum
	$parent clone $privatedb($parent)
	return $privatedb($parent)
}

array set ::dbi::sqlite3::typetrans {
	integer {integer 4}
	double {double 4}
	float {float 4}
}

proc ::dbi::sqlite3::info {db args} {
	upvar #0 ::dbi::sqlite3::typetrans typetrans
	set db [privatedb $db]
	set len [llength $args]
	if {$len < 1} {error "wrong # args: should be \"$db info option ...\""}
	set type [lindex $args 0]
	switch -exact $type {
		fields {
			if {$len == 2} {
				set table [lindex $args 1]
				return [::dbi::sqlite3::fieldsinfo $db $table]
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
				set list [$db exec -flat "pragma table_info(\"$table\")"]
				if {[llength $list] == 1} {set list ""}
				foreach {pos name type nullable default temp} $list {
					lappend fields $name
					lappend result otype,$name $type
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
				set list [$db exec -flat "pragma index_list(\"$table\")"]
				if {[llength $list] == 1} {set list ""}
				foreach {seq name unique} $list {
					set columns {}
					foreach {seqno cid cname} [$db exec -flat "pragma index_info('$name')"] {
						lappend columns $cname
					}
					if {$unique} {
						lappend result unique,$columns $name
					} else {
						lappend result index,$columns $name
					}
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

proc ::dbi::sqlite3::fieldsinfo {db table} {
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

proc ::dbi::sqlite3::update {db} {
	if {![llength [$db exec {select name from sqlite_master where name = '_dbi_serials'}]]} {
	} elseif {[lsearch [$db fields _dbi_serials] locked] == -1} {
		$db exec {
			alter table _dbi_serials add column locked integer default 0
		}
	}
}

proc ::dbi::sqlite3::serial_basic {db table field} {
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
				locked integer default 0,
				unique(stable,sfield)
			)
		}
	} elseif {[lsearch [$db fields _dbi_serials] locked] == -1} {
		$db exec {
			alter table _dbi_serials add column locked integer default 0
		}
	}
}

proc ::dbi::sqlite3::serial_shared {db table field} {
	upvar #0 ::dbi::sqlite3::shared shared
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

proc ::dbi::sqlite3::serial_add_trigger {db table field sharedtable sharedfield} {
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

proc ::dbi::sqlite3::serial_add {db table field args} {
	set db [privatedb $db]
	::dbi::sqlite3::serial_basic $db $table $field
	if [llength $args] {set current [lindex $args 0]} else {set current 0}
	set serial [$db exec -flat {select "serial" from _dbi_serials where "stable" = ? and "sfield" = ?} $table $field]
	if {[llength $serial]} {
		return -code error "serial already exists on field \"$field\" in table \"$table\""
	}
	$db exec {insert into _dbi_serials (stable,sfield,serial) values (?,?,?)} $table $field $current
	::dbi::sqlite3::serial_add_trigger $db $table $field $table $field
}

proc ::dbi::sqlite3::serial_share {db table field stable sfield} {
	upvar #0 ::dbi::sqlite3::shared shared
	set db [privatedb $db]
	serial_basic $db $table $field
	$db exec {insert into _dbi_serials (stable,sfield,serial,sharedtable,sharedfield) values (?,?,?,?,?)} $table $field -1 $stable $sfield
	::dbi::sqlite3::serial_add_trigger $db $table $field $stable $sfield
}

proc ::dbi::sqlite3::serial_delete {db table field} {
	set db [privatedb $db]
	serial_basic $db $table $field
	$db exec {delete from _dbi_serials where stable = ? and sfield = ?} $table $field
	set name _dbi_trigger_${table}_${field}
	catch {$db exec [subst {drop trigger $name}]}
}

proc ::dbi::sqlite3::serial_set {db table field args} {
	set db [privatedb $db]
	foreach {stable sfield} [::dbi::sqlite3::serial_shared $db $table $field] break
	if [llength $args] {
		$db exec [subst {update _dbi_serials set serial = ? where stable = ? and sfield = ?}] [lindex $args 0] $stable $field
	} else {
		return [$db exec {select serial from _dbi_serials where stable = ? and sfield = ?} $stable $field]
	}
}

proc ::dbi::sqlite3::serial_next {db table field} {
	set db [privatedb $db]
	foreach {stable sfield} [::dbi::sqlite3::serial_shared $db $table $field] break
	set num 1
	while {$num < 200} {
		set changed [$db exec {
			update _dbi_serials set serial = serial+1, locked = 1
			where stable = ? and sfield = ? and locked = 0
		} $stable $field]
		if {$changed} break
		after 3
		incr num
	}
	if {$changed} {
		set current [$db exec {
			select serial from _dbi_serials where stable = ? and sfield = ?
		} $stable $field]
		$db exec {
			update _dbi_serials set locked = 0
			where stable = ? and sfield = ?
		} $stable $field
	} else {
		$db begin
		$db exec {
			update _dbi_serials set serial = serial+1 where stable = ? and sfield = ?
		} $stable $field
		set current [$db exec {
			select serial from _dbi_serials where stable = ? and sfield = ?
		} $stable $field]
		$db exec {
			update _dbi_serials set locked = 0
			where stable = ? and sfield = ?
		} $stable $field
		$db commit
	}
	return $current
}

proc ::dbi::sqlite3::open_test {db file} {
	if {[string equal $file :memory:]} {return $file}
	if {![file exists $file]} {
		return -code error "file \"$file\" is not a valid database"
	}
	if {![string equal [file pathtype $file] absolute]} {
		set file [file join [pwd] $file]
	}
	return $file
}

proc ::dbi::sqlite3::list_common {list1 list2} {
	set result {}
	foreach el $list1 {
		if {[lsearch $list2 $el] != -1} {lappend result $el}
	}
	return $result
}

proc ::dbi::sqlite3::recreatetabledef {db table aVar newfields {delcol {}}} {
	upvar $aVar a
	set tabledef {}
	foreach field $newfields {
		if {[::info exists a(newdef,$field)]} {
			set temp "\"$field\" $a(newdef,$field)"
		} else {
			set temp "\"$field\" $a(otype,$field)"
			if {[::info exists a(notnull,$field)]} {append temp " not null"}
		}
		lappend tabledef $temp
	}
	foreach name [array names a primary,*] {
		set fields [string range $name 8 end]
		set fields [list_common $fields $newfields]
		if {![llength $fields]} continue
		lappend tabledef "primary key([join $fields ,])"
	}
	foreach name [array names a unique,*] {
		set fields [string range $name 7 end]
		if {[lsearch $fields $delcol] != -1} continue
		lappend tabledef "unique([join $fields ,])"
	}
	foreach name [array names a check,*] {
		set fields [string range $name 7 end]
		if {[lsearch $fields $delcol] != -1} continue
		lappend tabledef "unique([join $fields ,])"
	}
	foreach name [array names a foreign,*] {
		set fields [string range $name 8 end]
		if {![llength $fields]} continue
		if {[lsearch $fields $delcol] != -1} continue
		foreach {ftable ffields} $a($name) break
		lappend tabledef "foreign key(\"[join $fields {","}]\") references ${ftable}(\"[join $ffields {","}]\")"
	}
	return $tabledef
}

proc ::dbi::sqlite3::gettabledef {db table} {
	set sql [lindex [$db exec -flat {
		select sql from sqlite_master where type='table' and name=?;
	} $table] 0]
	if {![string length $sql]} {error "table $table not found"}
	regexp [subst {^CREATE TABLE "?$table"? \\((.*)\\)\$}] [string trim $sql] temp tabledef
	# split on ,
	set result {}
	set curline {}
	set bracesopen 0
	foreach line [split $tabledef ,] {
		if {($bracesopen == 0) && [string length $curline]} {
			lappend result [string trim $curline]
			set curline {}
		}
		set braces [expr {[regexp -all {\(} $line] - [regexp -all {\)} $line]}]
		incr bracesopen $braces
		if {[string length $curline]} {
			append curline ,$line
		} else {
			set curline $line
		}
	}
	if {[string length $curline]} {
		lappend result [string trim $curline]
	}
	return $result
}

proc ::dbi::sqlite3::dropcolumn {db table column args} {
	set columns [list $column]
	eval lappend columns $args
	array set a [$db info table $table]
	set newfields $a(fields)
	foreach column $columns {
		set pos [lsearch $newfields $column]
		if {$pos == -1} {error "no column $column present in table $table"}
		set newfields [lreplace $newfields $pos $pos]
	}
	set tabledef [::dbi::sqlite3::gettabledef $db $table]
	foreach column $columns {
		set pos 0
		foreach line $tabledef {
			if {[regexp "^\"?$column\"?\[ \t\n\]+" $line]} break
			incr pos
		}
		set tabledef [lreplace $tabledef $pos $pos]
	}
	$db begin
	$db exec [subst {create temp table temp_table as select "[join $newfields {","}]" from "$table"}]
	$db exec [subst {drop table "$table"}]
	$db exec [subst {create table "$table" ([join $tabledef ,\n])}]
	$db exec [subst {insert into "$table" select * from temp_table}]
	$db exec [subst {drop table temp_table}]
	$db commit
}

if 0 {
	set sql [lindex [$db exec -flat [subst {
		select sql from sqlite_master
		where type='table' and name='$table';
	}]] 0]
}

proc ::dbi::sqlite3::altercolumn {db table column newdef} {
	array set a [$db info table $table]
	set pos [lsearch $a(fields) $column]
	if {$pos == -1} {error "no column $column present in table $table"}
	set tabledef [::dbi::sqlite3::gettabledef $db $table]
	set pos 0
	foreach line $tabledef {
		if {[regexp "^\"?$column\"?\[ \t\n\]+" $line]} break
		incr pos
	}
	set tabledef [lreplace $tabledef $pos $pos "$column $newdef"]
	$db begin
	catch {$db exec [subst {drop table temp_table}]}
	$db exec [subst {create temp table temp_table as select "[join $a(fields) {","}]" from "$table"}]
	$db exec [subst {drop table "$table"}]
	$db exec [subst {create table "$table" ([join $tabledef ,\n])}]
	$db exec [subst {insert into "$table" select * from temp_table}]
	$db exec [subst {drop table temp_table}]
	$db commit
}
