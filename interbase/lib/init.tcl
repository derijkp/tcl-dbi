#
# Load libs
# ---------
#

package require dbi

namespace eval dbi::interbase {}

# $Format: "set ::dbi::interbase::version 0.$ProjectMajorVersion$"$
set ::dbi::interbase::version 0.8
# $Format: "set ::dbi::interbase::patchlevel $ProjectMinorVersion$"$
set ::dbi::interbase::patchlevel 5
package provide dbi_interbase $::dbi::interbase::version

proc ::dbi::interbase::init {name testcmd} {
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
		# Load the shared library
		#
		load $libfile
		catch {unset libbase}
	}
}
::dbi::interbase::init dbi_interbase dbi_interbase
rename ::dbi::interbase::init {}

array set ::dbi::interbase::typetrans {261 blob 14 char 40 cstring 11 d_float 27 double 10 float 16 int64 8 integer 9 quad 7 smallint 12 date 13 time 35 timestamp 37 varchar}

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

set ::dbi::interbase::privatedbnum 1
proc ::dbi::interbase::privatedb {db} {
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
	set privatedb($parent) ::dbi::interbase::priv_$privatedbnum
	incr privatedbnum
	$parent clone $privatedb($parent)
	return $privatedb($parent)
}

proc ::dbi::interbase::index_info {db index} {
	set relation [$db exec {
		select RDB$RELATION_NAME
		from RDB$INDICES
		where RDB$INDEX_NAME = ?} $index]
	if {![llength $relation]} {
		foreach {relation nindex} [lindex [$db exec {
			select RDB$RELATION_NAME,RDB$INDEX_NAME
			from RDB$RELATION_CONSTRAINTS
			where RDB$CONSTRAINT_NAME = ? } $index] 0] break
		if {[::info exists nindex]} {
			set index $nindex
		} else {
			set field [$db exec {
				select RDB$TRIGGER_NAME
				from RDB$CHECK_CONSTRAINTS
				where RDB$CONSTRAINT_NAME = ? } $index]
			return [list $relation $field]
		}
	}
	set field [$db exec {
		select RDB$FIELD_NAME
		from RDB$INDEX_SEGMENTS
		where RDB$INDEX_NAME = ? } $index]
	return [list $relation $field]
}

proc ::dbi::interbase::indexsegments {db index} {
	eval concat [$db exec {
		select RDB$FIELD_NAME from RDB$INDEX_SEGMENTS
		where RDB$INDEX_NAME = ?
		order by RDB$FIELD_POSITION
	} $index]
}

proc ::dbi::interbase::info {db args} {
	upvar #0 ::dbi::interbase::typetrans typetrans
	set db [privatedb $db]
	set len [llength $args]
	if {$len < 1} {error "wrong # args: should be \"$db info option ...\""}
	set type [lindex $args 0]
	switch -exact $type {
		user {
			set result [string toupper [$db exec {select user from rdb$database}]]
		}
		roles {
			if {$len == 1} {
				set result [$db exec {select RDB$ROLE_NAME from RDB$ROLES order by RDB$ROLE_NAME}]
			} elseif {$len == 2} {
				set user [string toupper [lindex $args 1]]
				set result [$db exec {
					select RDB$RELATION_NAME from RDB$USER_PRIVILEGES 
					where RDB$USER = ? and RDB$PRIVILEGE = 'M'
					order by RDB$RELATION_NAME
				} $user]
			} else {
				error "wrong # args: should be \"$db info roles ?username?\""
			}
		}
		fields {
			if {$len == 2} {
				set table [lindex $args 1]
				set c [$db exec {
					select RDB$FIELD_NAME,RDB$FIELD_SOURCE,RDB$NULL_FLAG
					from RDB$RELATION_FIELDS
					where RDB$RELATION_NAME = ?
					order by RDB$FIELD_POSITION} $table]
				set result ""
				foreach el $c {lappend result [lindex $el 0]}
				return $result
			} else {
				error "wrong # args: should be \"$db info fields ?table?\""
			}
		}
		tables {
			set result [$db exec -flat {select rdb$relation_name from rdb$relations where RDB$SYSTEM_FLAG = 0}]
		}
		systemtables {
			set result ""
			foreach line [$db exec {select rdb$relation_name from rdb$relations where RDB$SYSTEM_FLAG = 1}] {
				lappend result [lindex $line 0]
			}
		}
		views {
			set result ""
			foreach line [$db exec {select rdb$relation_name from rdb$relations where RDB$VIEW_BLR is not null}] {
				lappend result [lindex $line 0]
			}
		}
		domains {
			set list [$db exec {
				select RDB$FIELD_NAME from rdb$fields 
				where RDB$FIELD_NAME NOT LIKE 'RDB$%'
				AND RDB$SYSTEM_FLAG <> 1
	   			order BY RDB$FIELD_NAME
				}]
			set result ""
			foreach line $list {lappend result [lindex $line 0]}
		}
		domain {
			if {$len == 2} {
				set domain [lindex $args 1]
				set temp [lindex [$db exec {
					select RDB$FIELD_TYPE,RDB$FIELD_LENGTH,RDB$NULL_FLAG
					from RDB$FIELDS where RDB$FIELD_NAME = ?} $domain] 0]
				set result $typetrans([lindex $temp 0])
				switch $result {
					char - varchar {
						append result ([lindex $temp 1])
					}
				}
				if {![string length [lindex $temp end]]} {
					lappend result nullable
				} else {
					lappend result {not null}
				}
			} else {
				error "wrong # args: should be \"$db info domain ?domain?\""
			}
		}
		access {
			set option [lindex $args 1]
			switch $option {
				select {set char S}
				insert {set char I}
				delete {set char D}
				update {set char U}
				references {set char R}
				default {error "wrong option \"$option\": must be one of select, insert, delete, update or reference"}
			}
			if {$len == 3} {
				set user [string toupper [lindex $args 2]]
				set temp [$db exec {
					select distinct RDB$RELATION_NAME
					from RDB$USER_PRIVILEGES where RDB$USER = ? and RDB$PRIVILEGE = ?
					order by RDB$RELATION_NAME} $user $char]
				set result {}
				foreach table [eval concat $temp] {
					if {![string match RDB$* $table]} {
						lappend result $table
					}
				}
			} elseif {$len == 4} {
				set user [string toupper [lindex $args 2]]
				set table [lindex $args 3]
				set temp [$db exec {
					select RDB$FIELD_NAME
					from RDB$USER_PRIVILEGES where RDB$USER = ? and RDB$RELATION_NAME = ? and RDB$PRIVILEGE = ?} $user $table $char]
				if {([llength $temp] != 0) && ("[lindex $temp 0]" == "{}")} {
					set result [$db info fields $table]
				} else {
					set result {}
					set a() 1
					foreach field [eval concat $temp] {
						if {![::info exists a($field)]} {
							lappend result $field
							set a($field) 1
						}
					}
				}
			} else {
				error "wrong # args: should be \"$db info access select/update/delete/insert user/role ?table?\""
			}
		}
		table {
			if {$len == 2} {
				set table [lindex $args 1]
				set result ""
				set temp [$db exec {
					select rf.RDB$FIELD_NAME, f.RDB$FIELD_NAME, f.RDB$FIELD_TYPE, f.RDB$DIMENSIONS, 
						f.RDB$COMPUTED_BLR, f.RDB$COMPUTED_SOURCE,
						f.RDB$FIELD_LENGTH, rf.RDB$NULL_FLAG,
						rf.RDB$DEFAULT_SOURCE, f.RDB$DEFAULT_SOURCE, f.RDB$VALIDATION_SOURCE
					from RDB$RELATION_FIELDS rf, RDB$FIELDS f
					where rf.RDB$FIELD_SOURCE = f.RDB$FIELD_NAME and rf.RDB$RELATION_NAME = ?
					order by rf.RDB$FIELD_POSITION, rf.RDB$FIELD_NAME
				} $table]
				if {[llength $temp] == 0} {error "table \"$table\" does not exist"}
				set sresult ""
				set fields ""
				foreach line $temp {
					set field [lindex $line 0]
					lappend fields $field
					set type $typetrans([lindex $line 2])
					lappend result type,$field $type
					set dom [lindex $line 1]
					if {![string match {RDB$*} $dom]} {
						lappend result domain,$field $dom
					}
					foreach key {
						dim computed computed_src
						length notnull default fdefault validation
					} value [lrange $line 3 end] {
						if {[string length $value]} {
							lappend result $key,$field $value
						}
					}
				}
				set result [linsert $result 0 fields $fields]
				set temp [$db exec {
					select RDB$CONSTRAINT_TYPE, RDB$CONSTRAINT_NAME, RDB$INDEX_NAME
					from RDB$RELATION_CONSTRAINTS
					where RDB$RELATION_NAME = ?
				} $table]
				foreach line $temp {
					foreach {type name index} $line break
					switch $type {
						"PRIMARY KEY" {
							set field [::dbi::interbase::indexsegments $db $index]
							lappend result primary,$field $name
						}
						"UNIQUE" {
							set field [::dbi::interbase::indexsegments $db $index]
							lappend result unique,$field $name
						}
						"FOREIGN KEY" {
							set field [::dbi::interbase::indexsegments $db $index]
							set temp2 [lindex [$db exec {
								select rel.RDB$RELATION_NAME, rel.RDB$INDEX_NAME,
								ref.RDB$UPDATE_RULE, ref.RDB$DELETE_RULE
								from RDB$RELATION_CONSTRAINTS rel, RDB$REF_CONSTRAINTS ref
								where rel.RDB$CONSTRAINT_NAME = ref. RDB$CONST_NAME_UQ
									and ref.RDB$CONSTRAINT_NAME = ?
							} $name] 0]
							set reffield [::dbi::interbase::indexsegments $db [lindex $temp2 1]]
							set reftable [lindex $temp2 0]
							set rule [lindex $temp2 2]
							if {"$rule" != "RESTRICT"} {lappend sub update_rule $rule}
							set rule [lindex $temp2 3]
							if {"$rule" != "RESTRICT"} {lappend sub delete_rule $rule}
							lappend result foreign,$field [list $reftable $reffield]
						}
						"CHECK" {
							set ctemp [$db exec {
								select RDB$TRIGGER_SOURCE
								from RDB$TRIGGERS trg, RDB$CHECK_CONSTRAINTS chk
								where trg.RDB$TRIGGER_TYPE = 1
									and trg.RDB$TRIGGER_NAME = chk.RDB$TRIGGER_NAME
									and chk.RDB$CONSTRAINT_NAME = ?} $name]
							lappend result constraint,[lindex [lindex $ctemp 0] 0] $name
						}
					}
				}
			} else {
				error "wrong # args: should be \"$db info table tablename\""
			}
		}
		default {
			error "error: info about $type not supported"
		}
	}
	return $result
}

proc ::dbi::interbase::fieldsinfo {db table} {
	set db [privatedb $db]
	set table $table
	set c [$db exec {
		select RDB$FIELD_NAME,RDB$FIELD_SOURCE,RDB$NULL_FLAG
		from RDB$RELATION_FIELDS
		where RDB$RELATION_NAME = ?
		order by RDB$FIELD_POSITION} $table]
	if {![llength $c]} {
		return -code error "table \"$table\" does not exist"
	}
	set result ""
	foreach el $c {lappend result [lindex $el 0]}
	return $result
}

proc ::dbi::interbase::serial_add {db table field args} {
	upvar #0 ::dbi::interbase::typetrans typetrans
	set db [privatedb $db]
	set name srl\$${table}_${field}
	set btable $table
	if [llength $args] {set current [lindex $args 0]} else {set current 0}
	set fieldsource [lindex [lindex [$db exec {
		select RDB$FIELD_SOURCE
		from RDB$RELATION_FIELDS
		where RDB$RELATION_NAME = ? and RDB$FIELD_NAME = ?} $btable $field] 0] 0]
	if {![llength $fieldsource]} {
		set field $field
		set fieldsource [lindex [lindex [$db exec {
			select RDB$FIELD_SOURCE
			from RDB$RELATION_FIELDS
			where RDB$RELATION_NAME = ? and RDB$FIELD_NAME = ?} $btable $field] 0] 0]
		if {![llength $fieldsource]} {
			error "field \"$field\" not found in table \"$table\""
		}
	}
	set type [$db exec {
		select RDB$FIELD_TYPE
		from RDB$FIELDS
		where RDB$FIELD_NAME = ? } $fieldsource]
	set type $typetrans($type)
	$db exec [subst {
		create generator "$name";
		create trigger "$name" for "$table" before
		insert as
		begin
			if (NEW."$field" is null) then
			NEW."$field" = cast (gen_id("$name",1) as $type);
		end;
	}]
	$db exec [subst {
		set generator "$name" to $current
	}]
	return $current
}

proc ::dbi::interbase::serial_share {db table field stable sfield} {
	upvar #0 ::dbi::interbase::typetrans typetrans
	upvar #0 ::dbi::interbase::shared shared
	set db [privatedb $db]
	set triggername srl\$${table}_${field}
	set name srl\$${stable}_${sfield}
	set btable $table
	set fieldsource [lindex [lindex [$db exec {
		select RDB$FIELD_SOURCE
		from RDB$RELATION_FIELDS
		where RDB$RELATION_NAME = ? and RDB$FIELD_NAME = ?} $btable $field] 0] 0]
	if {![llength $fieldsource]} {
		set field $field
		set fieldsource [lindex [lindex [$db exec {
			select RDB$FIELD_SOURCE
			from RDB$RELATION_FIELDS
			where RDB$RELATION_NAME = ? and RDB$FIELD_NAME = ?} $btable $field] 0] 0]
		if {![llength $fieldsource]} {
			error "field \"$field\" not found in table \"$table\""
		}
	}
	set type [$db exec {
		select RDB$FIELD_TYPE
		from RDB$FIELDS
		where RDB$FIELD_NAME = ? } $fieldsource]
	set type $typetrans($type)
	$db exec [subst {
		create trigger "$triggername" for "$table" before
		insert as
		begin
			if (NEW."$field" is null) then
			NEW."$field" = cast (gen_id("$name",1) as $type);
		end;
	}]
	set shared($table,$field) $name
}

proc ::dbi::interbase::serial_delete {db table field} {
	set db [privatedb $db]
	set name [::dbi::interbase::serial_name $db $table $field]
	catch {$db exec "drop trigger \"$name\""}
	if {[string equal $name srl\$${table}_${field}]} {
		catch {$db exec {delete from rdb$generators where rdb$generator_name = ?} $name}
	}
}

proc ::dbi::interbase::serial_name {db table field} {
	upvar #0 ::dbi::interbase::shared shared
	if {![::info exists shared($table,$field)]} {
		set pattern {[\n\t ]*begin[\n\t ]*if \(NEW."([^"])+" is null\) then[\n\t ]*NEW."([^"])+" = cast \(gen_id\("srl\$([^_"]+)_([^_"]+)",1\) as integer\);[\n\t ]*end}
		foreach trigger [$db exec {
			select RDB$TRIGGER_SOURCE,RDB$RELATION_NAME
			from RDB$TRIGGERS where RDB$RELATION_NAME = ?
		} $table] {
			if {[regexp $pattern $trigger temp field1 field2 ttable field]} {
				set shared($table,$field) srl\$${ttable}_${field}
			}
		}
		if {![::info exists shared($table,$field)]} {
			set shared($table,$field) srl\$${table}_${field}
		}
	}
	return $shared($table,$field)
}

proc ::dbi::interbase::serial_set {db table field args} {
	set db [privatedb $db]
	set name [::dbi::interbase::serial_name $db $table $field]
	if [llength $args] {
		$db exec "set generator \"$name\" to [lindex $args 0]"
	} else {
		$db exec "select (gen_id(\"$name\",0)) from rdb\$database"
	}
}

proc ::dbi::interbase::serial_next {db table field} {
	set db [privatedb $db]
	set name [::dbi::interbase::serial_name $db $table $field]
	$db exec "select (gen_id(\"$name\",1)) from rdb\$database"
}

proc ::dbi::interbase::errorclean {db error} {
	if {[regexp {^violation of FOREIGN KEY constraint "([^"]+)"} $error temp index]} {
		foreach  {table field} [::dbi::interbase::index_info $db $index] break
		regsub {on table} $error [subst {on field "$field" in table}] error
	} elseif {[regexp {^violation of PRIMARY or UNIQUE KEY constraint "([^"]+)"} $error temp index]} {
		foreach  {table field} [::dbi::interbase::index_info $db $index] break
		regsub {on table} $error [subst {on field "$field" in table}] error
	}
	return $error
}
