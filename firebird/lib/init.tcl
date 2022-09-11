#
# Load libs
# ---------
#

namespace eval dbi::firebird {}

set ::dbi::firebird::version 2.1
set ::dbi::firebird::patchlevel 0
package provide dbi_firebird $::dbi::firebird::version

package require pkgtools
pkgtools::init $dbi::firebird::dir dbi_firebird

array set ::dbi::firebird::typetrans {
	7 smallint 8 integer 9 quad 10 float 11 d_float 12 date 13 time 14 char 16 int64 
	23 boolean 24 decfloat16 25 decfloat34 26 int128 27 double 28 time_w_zone 29 timestamp_w_zone
	35 timestamp 37 varchar 40 cstring 261 blob
}

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

set ::dbi::firebird::privatedbnum 1
proc ::dbi::firebird::privatedb {db} {
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
	set privatedb($parent) ::dbi::firebird::priv_$privatedbnum
	incr privatedbnum
	$parent clone $privatedb($parent)
	return $privatedb($parent)
}

proc ::dbi::firebird::index_info {db index} {
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

proc ::dbi::firebird::indexsegments {db index} {
	eval concat [$db exec {
		select RDB$FIELD_NAME from RDB$INDEX_SEGMENTS
		where RDB$INDEX_NAME = ?
		order by RDB$FIELD_POSITION
	} $index]
}

proc ::dbi::firebird::fieldsinfo {db table} {
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

#
# info
#

proc ::dbi::firebird::info {db args} {
	upvar #0 ::dbi::firebird::typetrans typetrans
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
				if {![llength $temp]} {error "domain \"$domain\" not found"}
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
				set result [info_table $db [lindex $args 1]]
			} else {
				error "wrong # args: should be \"$db info table tablename\""
			}
		}
		referenced {
			if {$len == 2} {
				set table [lindex $args 1]
				set result {}
				foreach {relation name index} [$db exec -flat {
					select  RDB$RELATION_NAME,RDB$CONSTRAINT_NAME, RDB$INDEX_NAME from "RDB$RELATION_CONSTRAINTS"
					where "RDB$CONSTRAINT_TYPE" =?
				} {FOREIGN KEY}] {
					set field [::dbi::firebird::indexsegments $db $index]
					set temp2 [lindex [$db exec {
						select rel.RDB$RELATION_NAME, rel.RDB$INDEX_NAME,
						ref.RDB$UPDATE_RULE, ref.RDB$DELETE_RULE
						from RDB$RELATION_CONSTRAINTS rel, RDB$REF_CONSTRAINTS ref
						where rel.RDB$CONSTRAINT_NAME = ref. RDB$CONST_NAME_UQ
							and ref.RDB$CONSTRAINT_NAME = ?
					} $name] 0]
					set reffield [::dbi::firebird::indexsegments $db [lindex $temp2 1]]
					set reftable [lindex $temp2 0]
					if {[string equal $reftable $table]} {
						lappend result $relation [list $field $reffield]
					}
				}
				return $result
			} else {
				error "wrong # args: should be \"$db info referenced tablename\""
			}
		}
		object {
			if {($len != 2)&&($len != 3)} {
				error "wrong # args: should be \"$db info dependencies object ?objecttype?\""
			}
			set result [::dbi::firebird::info_object $db [lindex $args 1] [lindex $args 2]]
		}
		dependencies {
			if {($len != 2)&&($len != 3)} {
				error "wrong # args: should be \"$db info dependencies table ?field?\""
			}
			set result [info_dependencies $db [lindex $args 1] [lindex $args 2]]
			return $result
		}
		fk {
			if {$len != 2} {
				error "wrong # args: should be \"$db info fk table\""
			}
			return [::dbi::firebird::list_fk $db [lindex $args 1]]
		}
		default {
			error "error: info about $type not supported"
		}
	}
	return $result
}

proc ::dbi::firebird::info_dependencies {db table {field {}}} {
	array set transtype {
		0 table 1 view 2 trigger 3 computed_field 4 validation 5 procedure
		6 expression_index 7 exception 8 user 9 field 10 index
	}
	set result {}
	if {![llength $field]} {
		set list [$db exec -flat {
			select  RDB$DEPENDENT_NAME, RDB$DEPENDED_ON_NAME, RDB$FIELD_NAME, RDB$DEPENDENT_TYPE, RDB$DEPENDED_ON_TYPE from "RDB$DEPENDENCIES"
			where "RDB$DEPENDED_ON_NAME" =?
		} $table]
	} else {
		set list [$db exec -flat {
			select  RDB$DEPENDENT_NAME, RDB$DEPENDED_ON_NAME, RDB$FIELD_NAME, RDB$DEPENDENT_TYPE, RDB$DEPENDED_ON_TYPE from "RDB$DEPENDENCIES"
			where "RDB$DEPENDED_ON_NAME" = ? and RDB$FIELD_NAME = ?
		} $table $field]
	}
	foreach {dependent depend_on field type type_on} $list {
		lappend result [list $field $transtype($type) $dependent]
	}
	set c [$db exec -flat {
		select src.RDB$RELATION_NAME,src.RDB$INDEX_NAME,
			ref.RDB$RELATION_NAME, ref.RDB$INDEX_NAME,
		link.RDB$UPDATE_RULE, link.RDB$DELETE_RULE
		from RDB$RELATION_CONSTRAINTS src, RDB$RELATION_CONSTRAINTS ref, RDB$REF_CONSTRAINTS link
		where ref.RDB$CONSTRAINT_NAME = link.RDB$CONST_NAME_UQ
			and src.RDB$CONSTRAINT_NAME = link.RDB$CONSTRAINT_NAME
			and link.RDB$CONSTRAINT_NAME = ?
	} $table]
	foreach {type name index} [$db exec -flat {
		select RDB$CONSTRAINT_TYPE,RDB$CONSTRAINT_NAME,RDB$INDEX_NAME
		from RDB$RELATION_CONSTRAINTS	where RDB$RELATION_NAME = ?
	} $table] {
		lappend result [list {} $type $name $index]
	}
	return $result
}

proc ::dbi::firebird::info_fk {db name} {
	set db [privatedb $db]
	set c [$db exec -flat {
		select src.RDB$RELATION_NAME,src.RDB$INDEX_NAME,
			ref.RDB$RELATION_NAME, ref.RDB$INDEX_NAME,
		link.RDB$UPDATE_RULE, link.RDB$DELETE_RULE
		from RDB$RELATION_CONSTRAINTS src, RDB$RELATION_CONSTRAINTS ref, RDB$REF_CONSTRAINTS link
		where ref.RDB$CONSTRAINT_NAME = link.RDB$CONST_NAME_UQ
			and src.RDB$CONSTRAINT_NAME = link.RDB$CONSTRAINT_NAME
			and link.RDB$CONSTRAINT_NAME = ?
	} $name]
	if {![llength $c]} {
		return -code error "relation_constraint \"$name\" not found"
	}
	set result {type foreignkey}
	foreach key {table index reftable refindex updaterule deleterule} value $c {
		lappend result $key $value
	}
	return $result
}

proc ::dbi::firebird::list_fk {db table} {
	set db [privatedb $db]
	set result "nothing dropped"
	set c [$db exec -flat {
		select src.RDB$CONSTRAINT_NAME,src.RDB$RELATION_NAME,src.RDB$INDEX_NAME,
			ref.RDB$RELATION_NAME, ref.RDB$INDEX_NAME
		from RDB$RELATION_CONSTRAINTS src, RDB$RELATION_CONSTRAINTS ref, RDB$REF_CONSTRAINTS link
		where ref.RDB$CONSTRAINT_NAME = link.RDB$CONST_NAME_UQ
			and src.RDB$CONSTRAINT_NAME = link.RDB$CONSTRAINT_NAME
			and src.RDB$RELATION_NAME = ?
	} $table]
	set result {}
	foreach {name ctable index reftable refindex} $c {
		array set ai [$db info object $index]
		array set refai [$db info object $refindex]
		lappend result [list $name $ctable $ai(fields) $index $reftable $refai(fields) $refindex]
	}
	return $result
}

proc ::dbi::firebird::info_relation_constraint {db name} {
	set db [privatedb $db]
	set c [$db exec -flat {
		select RDB$CONSTRAINT_TYPE,RDB$RELATION_NAME,RDB$INDEX_NAME
		from RDB$RELATION_CONSTRAINTS	where RDB$CONSTRAINT_NAME = ? 
	} $name]
	if {![llength $c]} {
		return -code error "relation_constraint \"$name\" not found"
	}
	set result {}
	foreach key {type table index} value $c {
		lappend result $key $value
	}
}

proc ::dbi::firebird::info_trigger {db trigger} {
	set db [privatedb $db]
	array set triggertrans {1 {before insert} 2 {after insert} 3 {before update} 4 {after update} 5 {before delete} 6 {after delete}}
	set c [$db exec -flat {
		select RDB$TRIGGER_NAME,RDB$RELATION_NAME,RDB$TRIGGER_SEQUENCE,RDB$TRIGGER_TYPE,RDB$TRIGGER_SOURCE,RDB$DESCRIPTION,RDB$TRIGGER_INACTIVE,RDB$SYSTEM_FLAG,RDB$FLAGS
		from RDB$TRIGGERS
		where RDB$TRIGGER_NAME = ?
	} $trigger]
	if {![llength $c]} {
		return -code error "trigger \"$trigger\" not found"
	}
	set result {type trigger}
	foreach key {trigger_name relation_name trigger_sequence trigger_type trigger_source description trigger_inactive system_flag flags} value $c {
		lappend result $key $value
	}
	return $result
}

proc ::dbi::firebird::info_view {db name} {
	info_table $db $name
}

proc ::dbi::firebird::info_table {db table} {
	upvar #0 ::dbi::firebird::typetrans typetrans
	set db [privatedb $db]
	# id it a view ?
	set temp [$db exec {
		select rdb$relation_name from rdb$relations 
		where rdb$relation_name = ? and RDB$VIEW_BLR is not null
	} $table]
	if {[llength $temp]} {
		set result {type view}
	} else {
		set result {type table}
	}
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
	set result [linsert $result 2 fields $fields]
	set temp [$db exec {
		select RDB$CONSTRAINT_TYPE, RDB$CONSTRAINT_NAME, RDB$INDEX_NAME
		from RDB$RELATION_CONSTRAINTS
		where RDB$RELATION_NAME = ?
	} $table]
	foreach line $temp {
		foreach {type name index} $line break
		switch $type {
			"PRIMARY KEY" {
				set field [::dbi::firebird::indexsegments $db $index]
				lappend result primary,$field $name
			}
			"UNIQUE" {
				set field [::dbi::firebird::indexsegments $db $index]
				lappend result unique,$field $name
			}
			"FOREIGN KEY" {
				set field [::dbi::firebird::indexsegments $db $index]
				set temp2 [lindex [$db exec {
					select rel.RDB$RELATION_NAME, rel.RDB$INDEX_NAME,
					ref.RDB$UPDATE_RULE, ref.RDB$DELETE_RULE
					from RDB$RELATION_CONSTRAINTS rel, RDB$REF_CONSTRAINTS ref
					where rel.RDB$CONSTRAINT_NAME = ref. RDB$CONST_NAME_UQ
						and ref.RDB$CONSTRAINT_NAME = ?
				} $name] 0]
				set reffield [::dbi::firebird::indexsegments $db [lindex $temp2 1]]
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
	return $result
}

proc ::dbi::firebird::info_check {db name} {
	set db [privatedb $db]
	set c [$db exec -flat {
		select trg.RDB$TRIGGER_NAME,trg.RDB$RELATION_NAME,trg.RDB$TRIGGER_SEQUENCE,trg.RDB$TRIGGER_SOURCE,trg.RDB$DESCRIPTION,trg.RDB$TRIGGER_INACTIVE,trg.RDB$SYSTEM_FLAG,trg.RDB$FLAGS
		from RDB$TRIGGERS trg, RDB$CHECK_CONSTRAINTS chk
		where trg.RDB$TRIGGER_TYPE = 1
			and trg.RDB$TRIGGER_NAME = chk.RDB$TRIGGER_NAME
			and chk.RDB$CONSTRAINT_NAME = ?
	} $name]
	if {![llength $c]} {
		return -code error "check \"$check\" not found"
	}
	set result {type check}
	foreach key {trigger_name relation_name trigger_sequence trigger_source description trigger_inactive system_flag flags} value $c {
		lappend result $key $value
	}
	return $result
}

proc ::dbi::firebird::info_exception {db name} {
	set db [privatedb $db]
	set c [$db exec -flat {
		select RDB$EXCEPTION_NUMBER,RDB$MESSAGE,RDB$DESCRIPTION,RDB$SYSTEM_FLAG
		from RDB$EXCEPTIONS where RDB$EXCEPTION_NAME = ?
	} $name]
	if {![llength $c]} {
		return -code error "exception \"$name\" not found"
	}
	set result {type exception}
	foreach key {number message description system} value $c {
		lappend result $key $value
	}
	return $result
}

proc ::dbi::firebird::info_computed_field {db name} {
	set db [privatedb $db]
	set c [$db exec -flat {
		select rf.RDB$RELATION_NAME,rf.RDB$FIELD_NAME, f.RDB$COMPUTED_SOURCE
		from RDB$RELATION_FIELDS rf, RDB$FIELDS f
		where rf.RDB$FIELD_SOURCE = f.RDB$FIELD_NAME and rf.RDB$FIELD_SOURCE = ?
	} $name]
	if {![llength $c]} {
		return -code error "computed_column \"$name\" not found"
	}
	set result {type computed_column}
	foreach key {table field computed} value $c {
		lappend result $key $value
	}
	return $result
}

proc ::dbi::firebird::info_index {db index} {
	set c [$db exec -flat {
		select RDB$RELATION_NAME,RDB$UNIQUE_FLAG,RDB$DESCRIPTION,RDB$INDEX_INACTIVE,
			RDB$FOREIGN_KEY,RDB$SYSTEM_FLAG,RDB$STATISTICS
		from RDB$INDICES	where RDB$INDEX_NAME = ?
	} $index]
	if {![llength $c]} {
		return -code error "index \"$index\" not found"
	}
	set result {type index }
	foreach key {table unique description inactive foreign system statistics} value $c {
		lappend result $key $value
	}
	lappend result fields [$db exec {
		select RDB$FIELD_NAME from RDB$INDEX_SEGMENTS	where RDB$INDEX_NAME = ? order by RDB$FIELD_POSITION
	} $index]
	return $result
}

proc ::dbi::firebird::info_object {db object {type {}}} {
	set db [privatedb $db]
	if {[string length $type]} {
		return [::dbi::firebird::info_$type $db $object]
	} else {
		# todo:	validation procedure expression_index user field
		foreach type {
			check trigger table view computed_field exception index fk relation_constraint
		} {
			if {![catch {::dbi::firebird::info_$type $db $object} result]} {
				return $result
			}
		}
	}
}

#
# serial
#

proc ::dbi::firebird::serial_add {db table field args} {
	upvar #0 ::dbi::firebird::typetrans typetrans
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
	set ::dbi::firebird::shared($table,$field) $name
	return $current
}

proc ::dbi::firebird::serial_share {db table field stable sfield} {
	upvar #0 ::dbi::firebird::typetrans typetrans
	upvar #0 ::dbi::firebird::shared shared
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

proc ::dbi::firebird::serial_delete {db table field} {
	set db [privatedb $db]
	set name [::dbi::firebird::serial_name $db $table $field]
	catch {$db exec "drop trigger \"$name\""}
	if {[string equal $name srl\$${table}_${field}]} {
		catch {$db exec {delete from rdb$generators where rdb$generator_name = ?} $name}
	}
	set ::dbi::firebird::shared($table,$field) error
}

proc ::dbi::firebird::serial_name {db table field} {
	upvar #0 ::dbi::firebird::shared shared
	if {![::info exists shared($table,$field)]} {
		foreach tempfield [$db fields $table] {
			set shared($table,$tempfield) error
		}
		set pattern {[\n\t ]*begin[\n\t ]*if \(NEW."([^"]+)" is null\) then[\n\t ]*NEW."([^"]+)" = cast \(gen_id\("(srl\$[^"]+)",1\) as integer\);[\n\t ]*end}
		foreach trigger [$db exec {
			select RDB$TRIGGER_SOURCE,RDB$RELATION_NAME
			from RDB$TRIGGERS where RDB$RELATION_NAME = ?
		} $table] {
			if {[regexp $pattern $trigger temp field1 field2 name]} {
				set shared($table,$field2) $name
			}
		}
	}
	if {[string equal $shared($table,$field) error]} {
		return -code error "field \"$field\" in table \"$table\" is not a serial"
	} else {
		return $shared($table,$field)
	}
}

proc ::dbi::firebird::serial_set {db table field args} {
	set db [privatedb $db]
	set name [::dbi::firebird::serial_name $db $table $field]
	if [llength $args] {
		$db exec "set generator \"$name\" to [lindex $args 0]"
	} else {
		$db exec "select (gen_id(\"$name\",0)) from rdb\$database"
	}
}

proc ::dbi::firebird::serial_next {db table field} {
	set db [privatedb $db]
	set name [::dbi::firebird::serial_name $db $table $field]
	$db exec "select (gen_id(\"$name\",1)) from rdb\$database"
}

proc ::dbi::firebird::errorclean {db error} {
	catch {
		if {[regexp {^violation of FOREIGN KEY constraint "([^"]+)"} $error temp index]} {
			foreach  {table field} [::dbi::firebird::index_info $db $index] break
			regsub {on table} $error [subst {on field "$field" in table}] error
		} elseif {[regexp {^violation of PRIMARY or UNIQUE KEY constraint "([^"]+)"} $error temp index]} {
			foreach  {table field} [::dbi::firebird::index_info $db $index] break
			regsub {on table} $error [subst {on field "$field" in table}] error
		}
	}
	return $error
}




if 0 {
	proc ::dbi::firebird::dumptable {db table} {
		foreach line [$db exec "select * from \"$table\""] {
			puts $line
		}
		return {}
	}
	
	proc ::dbi::firebird::searchtable {db table pattern} {
		foreach line [$db exec "select * from \"$table\""] {
			if {[regexp $pattern $line]} {
				puts $line
			}
		}
		return {}
	}
	
	proc ::dbi::firebird::parse_systemtables {db pattern} {
		set db [privatedb $db]
		set systables [$db info systemtables]
		set result {}
		foreach table $systables {
			set c [$db exec "select * from $table"]
			foreach line $c {
				if {[regexp $pattern $c]} {
					lappend result $table
				}
			}
		}
		return [lsort -unique $result]
	}
}
