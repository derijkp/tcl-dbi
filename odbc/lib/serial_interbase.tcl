array set ::dbi::odbc::interbase_typetrans {261 blob 14 char 40 cstring 11 d_float 27 double 10 float 16 int64 8 integer 9 quad 7 smallint 12 date 13 time 35 timestamp 37 varchar}

proc ::dbi::odbc::serial_Interbase_add {db version table field args} {
	upvar #0 ::dbi::odbc::interbase_typetrans typetrans
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

proc ::dbi::odbc::serial_Interbase_delete {db version table field} {
	set name srl\$${table}_${field}
	$db exec {delete from rdb$generators where rdb$generator_name = ?} $name
	$db exec "drop trigger \"$name\""
}

proc ::dbi::odbc::serial_Interbase_set {db version table field args} {
	set name srl\$${table}_${field}
	if [llength $args] {
		$db exec "set generator \"$name\" to [lindex $args 0]"
	} else {
		$db exec "select (gen_id(\"$name\",0)) from rdb\$database"
	}
}

proc ::dbi::odbc::serial_Interbase_next {db version table field} {
	set name srl\$${table}_${field}
	$db exec "select (gen_id(\"$name\",1)) from rdb\$database"
}
