proc ::dbi::odbc_info {db args} {
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
				reference {set char R}
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

