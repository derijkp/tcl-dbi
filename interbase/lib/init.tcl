#
# Load libs
# ---------
#

package require dbi

namespace eval dbi::interbase {}

# $Format: "set ::dbi::interbase::version 0.$ProjectMajorVersion$"$
set ::dbi::interbase::version 0.0
# $Format: "set ::dbi::interbase::patchlevel $ProjectMinorVersion$"$
set ::dbi::interbase::patchlevel 10
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
		# Load the shared library if present
		# If not, Tcl code will be loaded when necessary
		#
		if [file exists $libfile] {
			if {"[info commands $testcmd]" == ""} {
				load $libfile
			}
		} else {
			set noc 1
			source [file join ${dir} lib listnoc.tcl]
		}
		catch {unset libbase}
	}
}
::dbi::interbase::init dbi_interbase dbi_interbase
rename ::dbi::interbase::init {}

array set ::dbi::interbase_typetrans {261 blob 14 char 40 cstring 11 d_float 27 double 10 float 16 int64 8 integer 9 quad 7 smallint 12 date 13 time 35 timestamp 37 varchar}

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

proc ::dbi::interbase_info {db args} {
	upvar #0 ::dbi::interbase_typetrans typetrans
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
				#set user [string toupper [lindex $args 1]]
				set user [lindex $args 1]
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
			set result [$db exec {select rdb$relation_name from rdb$relations where RDB$SYSTEM_FLAG = 0}]
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
				#set user [string toupper [lindex $args 2]]
				set user [lindex $args 2]
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
				#set user [string toupper [lindex $args 2]]
				set user [lindex $args 2]
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
							set field [::dbi::interbase_indexsegments $db $index]
							lappend result primary,$field $name
						}
						"UNIQUE" {
							set field [::dbi::interbase_indexsegments $db $index]
							lappend result unique,$field $name
						}
						"FOREIGN KEY" {
							set field [::dbi::interbase_indexsegments $db $index]
							set temp2 [lindex [$db exec {
								select rel.RDB$RELATION_NAME, rel.RDB$INDEX_NAME,
								ref.RDB$UPDATE_RULE, ref.RDB$DELETE_RULE
								from RDB$RELATION_CONSTRAINTS rel, RDB$REF_CONSTRAINTS ref
								where rel.RDB$CONSTRAINT_NAME = ref. RDB$CONST_NAME_UQ
									and ref.RDB$CONSTRAINT_NAME = ?
							} $name] 0]
							set reffield [::dbi::interbase_indexsegments $db [lindex $temp2 1]]
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

proc ::dbi::interbase_indexsegments {db index} {
	eval concat [$db exec {
		select RDB$FIELD_NAME from RDB$INDEX_SEGMENTS
		where RDB$INDEX_NAME = ?
		order by RDB$FIELD_POSITION
	} $index]
}

proc ::dbi::interbase_fieldsinfo {db table} {
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

#proc ::dbi::interbase_tableinfo {db table var} {
#	upvar $var data
#	upvar #0 ::dbi::interbase_typetrans typetrans
#	# Get table info
#	catch {unset data}
#	set table $table
#	set c [$db exec {
#		select RDB$RELATION_ID,RDB$OWNER_NAME from RDB$RELATIONS
#		where RDB$RELATION_NAME = ?} $table]
#	if ![llength $c] {error "table \"$table does not exist"}
#	foreach {id data(owner)} [lindex $c 0] break
#	set data(owner) [string tolower $data(owner)]
#	# Get fields and field info
#	set c [$db exec {
#		select RDB$FIELD_NAME,RDB$FIELD_SOURCE,RDB$NULL_FLAG
#		from RDB$RELATION_FIELDS
#		where RDB$RELATION_NAME = ?
#		order by RDB$FIELD_POSITION} $table]
#	foreach line $c {
#		foreach {field field_source notnull} $line break
#		set ofield [string tolower $field]
#		lappend data(fields) $ofield
#		set data(field,$ofield,notnull) [? {$notnull == 1} 1 0]
#		set c [$db exec {
#			select RDB$FIELD_LENGTH,RDB$FIELD_PRECISION,RDB$FIELD_SCALE,RDB$FIELD_TYPE
#			from RDB$FIELDS
#			where RDB$FIELD_NAME = ? } $field_source]
#		foreach {length precision scale fieldtype} [lindex $c 0] break
#		set data(field,$ofield,type) $typetrans($fieldtype)
#		switch $typetrans($fieldtype) {
#			char - varchar {
#				set data(field,$ofield,ftype) $typetrans($fieldtype)($length)
#			}
#			default {
#				set data(field,$ofield,ftype) $typetrans($fieldtype)
#			}
#		}
#		set data(field,$ofield,size) $length
#	}
#	# Get constraints information
#	set c [$db exec {
#		select RDB$CONSTRAINT_NAME,RDB$CONSTRAINT_TYPE,RDB$INDEX_NAME
#		from RDB$RELATION_CONSTRAINTS
#		where RDB$RELATION_NAME = ? } $table]
#	foreach line $c {
#		foreach {name type index} $line break
#		switch $type {
#			{NOT NULL} {
#				foreach {dest_table dest_field} [::dbi::interbase_index_info $db $name] break
#				set data(field,[string tolower $dest_field],notnull) 1
#			}
#			{UNIQUE} {
#				foreach {dest_table dest_field} [::dbi::interbase_index_info $db $name] break
#				set data(field,[string tolower $dest_field],unique) 1
#			}
#			{PRIMARY KEY} {
#				foreach {dest_table dest_field} [::dbi::interbase_index_info $db $name] break
#				set data(field,[string tolower $dest_field],primarykey) 1
#			}
#			{FOREIGN KEY} {
#				set c [$db exec {
#					select RDB$UPDATE_RULE,RDB$DELETE_RULE
#					from RDB$REF_CONSTRAINTS
#					where RDB$CONSTRAINT_NAME = ? } $name]
#				foreach {src_table src_field} [::dbi::interbase_index_info $db $index] break
#				set dest_index [$db exec {
#					select RDB$CONST_NAME_UQ
#					from RDB$REF_CONSTRAINTS
#					where RDB$CONSTRAINT_NAME = ? } $name]
#				foreach {dest_table dest_field} [::dbi::interbase_index_info $db $dest_index] break
#				set data(foreignkey,[string tolower $src_field]) [list [string tolower $dest_table] [string tolower $dest_field]]
#			}
#			{CHECK} {
#				set triggers [$db exec {
#					select RDB$TRIGGER_NAME
#					from RDB$CHECK_CONSTRAINTS
#					where RDB$CONSTRAINT_NAME = ? } $name]
#				foreach trigger $triggers {
#					set triggerdone($trigger) 1
#					set source [lindex [lindex [$db exec {
#						select RDB$TRIGGER_SOURCE
#						from RDB$TRIGGERS
#						where RDB$TRIGGER_NAME = ? } $trigger] 0] 0]
#					set data(check,$name) $source
#				}
#			}
#		}
#	}
#	# Get index information
#	set c [$db exec -nullvalue 0 {
#		select RDB$INDEX_NAME,RDB$UNIQUE_FLAG,RDB$FOREIGN_KEY
#		from RDB$INDICES
#		where RDB$RELATION_NAME = ? } $table]
#	foreach line $c {
#		foreach {name unique foreign} $line break
#		# if {"$foreign" != "0"} continue
#		set fields [$db exec {
#			select RDB$FIELD_NAME
#			from RDB$INDEX_SEGMENTS
#			where RDB$INDEX_NAME = ? } $name]
#		set name [string tolower $name]
#		set data(index,[string tolower $fields],name) $name
#		set data(index,$name,isunique) $unique
#		set data(index,$name,isforeign) [? {"$foreign" != "0"} 1 0]
#	}
#	# Get trigger information
#	set c [$db exec {
#		select RDB$TRIGGER_NAME,RDB$TRIGGER_SEQUENCE,RDB$TRIGGER_TYPE,RDB$TRIGGER_SOURCE
#		from RDB$TRIGGERS
#		where RDB$RELATION_NAME = ? } $table]
#	foreach line $c {
#		foreach {name sequence type source} $line break
#		if [::info exists triggerdone($name)] continue
#		set name [string tolower $name]
#		set data(trigger,$name,sequence) $sequence
#		set data(trigger,$name,type) [lindex {{} {before insert} {after insert} {before update} {after update} {before delete} {after delete}} $type]
#		set data(trigger,$name,source) $source
#	}
#}

proc ::dbi::interbase_serial_add {db table field args} {
	upvar #0 ::dbi::interbase_typetrans typetrans
	set name srl\$${table}_${field}
	set btable $table
	if [llength $args] {set current [lindex $args 0]} else {set current 1}
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
	$db exec "
		create generator \"$name\"; \n\
		create trigger \"$name\" for \"$table\" before \n\
		insert as \n\
		begin \n\
			if (NEW.\"$field\" is null) then
			NEW.\"$field\" = cast (gen_id(\"$name\",1) as $type); \n\
		end; \n\
		set generator \"$name\" to $current \
	"
	return $current
}

proc ::dbi::interbase_serial_delete {db table field} {
	set name srl\$${table}_${field}
	$db exec {delete from rdb$generators where rdb$generator_name = ?} $name
	$db exec "drop trigger \"$name\""
}

proc ::dbi::interbase_serial_set {db table field args} {
	set name srl\$${table}_${field}
	if [llength $args] {
		$db exec "set generator \"$name\" to [lindex $args 0]"
	} else {
		$db exec "select (gen_id(\"$name\",0)) from rdb\$database"
	}
}

proc ::dbi::interbase_serial_next {db table field} {
	set name srl\$${table}_${field}
	$db exec "select (gen_id(\"$name\",1)) from rdb\$database"
}
