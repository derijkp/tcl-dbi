proc ::dbi::odbc_info {db args} {
	set len [llength $args]
	if {$len < 1} {error "wrong # args: should be \"$db info option ...\""}
	set type [lindex $args 0]
	switch -exact $type {
		user {
			set result [$db sqlgetinfo user_name]
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
			set data [$db sqltables {} {} {} TABLE]
			set result {}
			foreach line $data {
				lappend result [lindex $line 2]
			}
		}
		systemtables {
			set data [$db sqltables {} {} {} {SYSTEM TABLE}]
			set result {}
			foreach line $data {
				lappend result [lindex $line 2]
			}
		}
		views {
			set data [$db sqltables {} {} {} VIEW]
			set result {}
			foreach line $data {
				lappend result [lindex $line 2]
			}
		}
		access {
			set option [lindex $args 1]
			set option [string toupper $option]
			if {$len == 3} {
				foreach table [$db info systemtables] {
					set excl($table) 1
				}
				set user [lindex $args 2]
				set temp [$db sqltableprivileges {} {} {}]
				set result {}
				foreach line $temp {
					set table [lindex $line 2]
					if {[::info exists excl($table)]} continue
					set grantee [lindex $line 4]
					if {[string equal $grantee $user]} {
						set privilege [lindex $line 5]
						if {[string equal $privilege $option]} {
							lappend result $table
						}
					}
				}
			} elseif {$len == 4} {
				set user [lindex $args 2]
				set table [lindex $args 3]
				set result {}
				set temp [$db sqlcolumnprivileges {} {} $table {}]
				if {![llength $temp]} {
					set temp [$db sqltableprivileges {} {} $table]
					foreach line $temp {
						set grantee [lindex $line 4]
						if {[string equal $grantee $user]} {
							set privilege [lindex $line 5]
							if {[string equal $privilege $option]} {
								set result [$db fields $table]
							}
						}
					}
				} else {
					foreach line $temp {
						set grantee [lindex $line 5]
						if {[string equal $grantee $user]} {
							set privilege [lindex $line 6]
							if {[string equal $privilege $option]} {
								lappend result [lindex $line 3]
							}
						}
					}
				}
			} else {
				error "wrong # args: should be \"$db info access select/update/delete/insert user/role ?table?\""
			}
		}
		table {
			if {$len != 2} {
				error "wrong # args: should be \"$db info table tablename\""
			}
			set table [lindex $args 1]
			set result ""
			set fields {}
			set temp [$db sqlcolumns {} {} $table {}]
			foreach line $temp {
				foreach {
					temp temp temp field typenum typename size bufferlength 
					digits radix nullable remarks default datatype
					datetimesub octetlength ordinal isnullable
				} $line break
				lappend fields $field
				switch -exact $typename {
					{CHARACTER VARYING} {
						set type varchar
						set length $size
					}
					{DOUBLE PRECISION} {
						set type double
						set length $size
					}
					CHAR {
						set type char
						set length $size
					}
					default {
						set type [string tolower $typename]
						set length $bufferlength
					}
				}
				lappend result type,$field $type
				lappend result length,$field $length
				if {![string equal $default {}]} {
					lappend result default,$field $default
				}
				if {[string equal $isnullable NO]} {
					lappend result notnull,$field 1
				}
			}
			lappend result fields $fields
			set temp [$db sqlprimarykeys {} {} $table]
			foreach line $temp {
				lappend result primary,[lindex $line 3] 1
			}
			set temp [$db sqlforeignkeys {} {} {} {} {} $table]
			foreach line $temp {
				lappend result foreign,[lindex $line 7] [lrange $line 2 3]
			}
			# still have to find check and unique constraints
		}
		default {
			error "error: info about $type not supported"
		}
	}
	return $result
}

