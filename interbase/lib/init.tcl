#
# Load libs
# ---------
#

package require dbi

# $Format: "set ::dbi::version 0.$ProjectMajorVersion$"$
set ::dbi::version 0.0

package provide dbi_interbase $::dbi::interbase_version

if {[lsearch [dbi info typesloaded] interbase] == -1} {
	_package_loadlib dbi_interbase [set ::dbi::interbase_version] $_package_dbi_interbase(library)
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

proc ::dbi::interbase_index_info {db index} {
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

proc ::dbi::interbase_info {db type args} {
	set systables [$db exec {select rdb$relation_name from rdb$relations where rdb$relation_name starting with 'RDB$'}]
	set systables [join $systables]
	set f [open /tmp/temp w]
	foreach table $systables {
		puts $f $table
		puts $f [join [$db exec "select * from $table"] \n]
	}
	close $f
}

proc ::dbi::interbase_tableinfo {db table var} {
	upvar $var data
	# Get table info
	catch {unset data}
	set table [string toupper $table]
	set c [$db exec {
		select RDB$RELATION_ID,RDB$OWNER_NAME from RDB$RELATIONS
		where RDB$RELATION_NAME = ?} $table]
	if ![llength $c] {error "table \"$table does not exist"}
	foreach {id data(owner)} [lindex $c 0] break
	set data(owner) [string tolower $data(owner)]
	# Get fields and field info
	set c [$db exec {
		select RDB$FIELD_NAME,RDB$FIELD_SOURCE,RDB$NULL_FLAG
		from RDB$RELATION_FIELDS
		where RDB$RELATION_NAME = ?
		order by RDB$FIELD_POSITION} $table]
	array set typetrans {261 blob 14 char 40 cstring 11 d_float 27 double 10 float 16 int64 8 integer 9 quad 7 smallint 12 date 13 time 35 timestamp 37 varchar}
	foreach line $c {
		foreach {field field_source notnull} $line break
		set ofield [string tolower $field]
		lappend data(fields) $ofield
		set data(field,$ofield,notnull) [? {$notnull == 1} 1 0]
		set c [$db exec {
			select RDB$FIELD_LENGTH,RDB$FIELD_PRECISION,RDB$FIELD_SCALE,RDB$FIELD_TYPE
			from RDB$FIELDS
			where RDB$FIELD_NAME = ? } $field_source]
		foreach {length precision scale fieldtype} [lindex $c 0] break
		set data(field,$ofield,type) $typetrans($fieldtype)
		switch $typetrans($fieldtype) {
			char - varchar {
				set data(field,$ofield,ftype) $typetrans($fieldtype)($length)
			}
			default {
				set data(field,$ofield,ftype) $typetrans($fieldtype)
			}
		}
		set data(field,$ofield,size) $length
	}
	# Get constraints information
	set c [$db exec {
		select RDB$CONSTRAINT_NAME,RDB$CONSTRAINT_TYPE,RDB$INDEX_NAME
		from RDB$RELATION_CONSTRAINTS
		where RDB$RELATION_NAME = ? } $table]
	foreach line $c {
		foreach {name type index} $line break
		switch $type {
			{NOT NULL} {
				foreach {dest_table dest_field} [::dbi::interbase_index_info $db $name] break
				set data(field,[string tolower $dest_field],notnull) 1
			}
			{UNIQUE} {
				foreach {dest_table dest_field} [::dbi::interbase_index_info $db $name] break
				set data(field,[string tolower $dest_field],unique) 1
			}
			{PRIMARY KEY} {
				foreach {dest_table dest_field} [::dbi::interbase_index_info $db $name] break
				set data(field,[string tolower $dest_field],primarykey) 1
			}
			{FOREIGN KEY} {
				set c [$db exec {
					select RDB$UPDATE_RULE,RDB$DELETE_RULE
					from RDB$REF_CONSTRAINTS
					where RDB$CONSTRAINT_NAME = ? } $name]
				foreach {src_table src_field} [::dbi::interbase_index_info $db $index] break
				set dest_index [$db exec {
					select RDB$CONST_NAME_UQ
					from RDB$REF_CONSTRAINTS
					where RDB$CONSTRAINT_NAME = ? } $name]
				foreach {dest_table dest_field} [::dbi::interbase_index_info $db $dest_index] break
				set data(foreignkey,[string tolower $src_field]) [list [string tolower $dest_table] [string tolower $dest_field]]
			}
			{CHECK} {
				set triggers [$db exec {
					select RDB$TRIGGER_NAME
					from RDB$CHECK_CONSTRAINTS
					where RDB$CONSTRAINT_NAME = ? } $name]
				foreach trigger $triggers {
					set triggerdone($trigger) 1
					set source [lindex [lindex [$db exec {
						select RDB$TRIGGER_SOURCE
						from RDB$TRIGGERS
						where RDB$TRIGGER_NAME = ? } $trigger] 0] 0]
					set data(check,$name) $source
				}
			}
		}
	}
	# Get index information
	set c [$db exec -nullvalue 0 {
		select RDB$INDEX_NAME,RDB$UNIQUE_FLAG,RDB$FOREIGN_KEY
		from RDB$INDICES
		where RDB$RELATION_NAME = ? } $table]
	foreach line $c {
		foreach {name unique foreign} $line break
		# if {"$foreign" != "0"} continue
		set fields [$db exec {
			select RDB$FIELD_NAME
			from RDB$INDEX_SEGMENTS
			where RDB$INDEX_NAME = ? } $name]
		set name [string tolower $name]
		set data(index,[string tolower $fields],name) $name
		set data(index,$name,isunique) $unique
		set data(index,$name,isforeign) [? {"$foreign" != "0"} 1 0]
	}
	# Get trigger information
	set c [$db exec {
		select RDB$TRIGGER_NAME,RDB$TRIGGER_SEQUENCE,RDB$TRIGGER_TYPE,RDB$TRIGGER_SOURCE
		from RDB$TRIGGERS
		where RDB$RELATION_NAME = ? } $table]
	foreach line $c {
		foreach {name sequence type source} $line break
		if [::info exists triggerdone($name)] continue
		set name [string tolower $name]
		set data(trigger,$name,sequence) $sequence
		set data(trigger,$name,type) [lindex {{} {before insert} {after insert} {before update} {after update} {before delete} {after delete}} $type]
		set data(trigger,$name,source) $source
	}
}

proc ::dbi::interbase_serial_add {db table field args} {
	set name srl\$${table}_${field}
	set btable [string toupper $table]
	if [llength $args] {set current [lindex $args 0]} else {set current 0}
	set fieldsource [lindex [lindex [$db exec {
		select RDB$FIELD_SOURCE
		from RDB$RELATION_FIELDS
		where RDB$RELATION_NAME = ? and RDB$FIELD_NAME = ?} $btable $field] 0] 0]
	if {![llength $fieldsource]} {
		set field [string toupper $field]
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
	array set typetrans {261 blob 14 char 40 cstring 11 d_float 27 double 10 float 16 int64 8 integer 9 quad 7 smallint 12 date 13 time 35 timestamp 37 varchar}
	set type $typetrans($type)
	db exec "
		create generator $name; \n\
		create trigger $name for $table before \n\
		insert as \n\
		begin \n\
			if (NEW.\"$field\" is null) then
			NEW.\"$field\" = cast (gen_id($name,1) as $type); \n\
		end; \n\
		set generator $name to $current \
	"
}

proc ::dbi::interbase_serial_delete {db table field} {
	set name [string toupper srl\$${table}_${field}]
	db exec {delete from rdb$generators where rdb$generator_name = ?} $name
	db exec "drop trigger $name"
}

proc ::dbi::interbase_serial_set {db table field args} {
	set name [string toupper srl\$${table}_${field}]
	if [llength $args] {
		db exec "set generator $name to [lindex $args 0]"
	} else {
		db exec "select (gen_id($name,0)) from rdb\$database"
	}
}

proc ::dbi::interbase_serial_next {db table field} {
	set name [string toupper srl\$${table}_${field}]
	db exec "select (gen_id($name,1)) from rdb\$database"
}
