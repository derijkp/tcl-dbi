#
# Load libs
# ---------
#

package require dbi

# $Format: "set ::dbi::version 0.$ProjectMajorVersion$"$
set ::dbi::version 0.0

package provide dbi_postgresql $::dbi::postgresql_version

if {[lsearch [dbi info typesloaded] postgresql] == -1} {
	_package_loadlib dbi_postgresql [set ::dbi::postgresql_version] $_package_dbi_postgresql(library)
}

#
# Procs
# -----
#
proc ::dbi::? {expr truevalue falsevalue} {
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

proc ::dbi::postgresql_fieldsinfo {db table} {
	set c [$db exec "\
		select attnum,attname,typname,attlen,attnotnull,atttypmod,usename,usesysid,pg_class.oid,relpages,reltuples,relhaspkey,relhasrules,relacl \
		from pg_class,pg_user,pg_attribute,pg_type \
		where (pg_class.relname='$table') and (pg_class.oid=pg_attribute.attrelid) and (pg_class.relowner=pg_user.usesysid) and (pg_attribute.atttypid=pg_type.oid) \
		order by attnum"] 
	if ![llength $c] {error "table \"$table\" does not exist"}
	set result ""
	foreach line [lrange $c 6 end] {
		lappend result [lindex $line 1]
	}
	return $result
}

proc ::dbi::postgresql_tableinfo {db table var} {
	upvar $var data
	catch {unset data}
	set c [$db exec "\
		select attnum,attname,typname,attlen,attnotnull,atttypmod,usename,usesysid,pg_class.oid,relpages,reltuples,relhaspkey,relhasrules,relacl \
		from pg_class,pg_user,pg_attribute,pg_type \
		where (pg_class.relname='$table') and (pg_class.oid=pg_attribute.attrelid) and (pg_class.relowner=pg_user.usesysid) and (pg_attribute.atttypid=pg_type.oid) \
		order by attnum"] 
	if ![llength $c] {error "table \"$table\" does not exist"}
	foreach {attnum attname typname attlen attnotnull atttypmod usename usesysid pg_class.oid relpages reltuples relhaspkey relhasrules relacl} [lindex $c 0] break
	set data(owner) $usename
	set data(oid) ${pg_class.oid}
	set data(ownerid) $usesysid
	set data(numtuples) $reltuples
	set data(numpages) $relpages
	set data(permissions) $relacl
	set data(hasrules) [? {"$relhasrules" == "t"} 1 0]
	array set typetrans {bpchar char}
	foreach line [lrange $c 6 end] {
		set field [lindex $line 1]
		lappend data(fields) $field
		foreach {attnum attname typname attlen attnotnull atttypmod usename usesysid pg_class.oid relpages reltuples relhaspkey relhasrules relacl} $line break
		if [::info exists typetrans($typname)] {
			set data(field,$field,type) $typetrans($typname)
		} else {
			set data(field,$field,type) $typname
		}
		set data(field,$field,size) $attlen
		set data(field,$field,fsize) $atttypmod
		set data(field,$field,notnull) [? {"$attnotnull" == "t"} 1 0]
	}
	foreach line [$db exec "select oid,indkey,indexrelid,indisprimary,indisunique from pg_index where (pg_class.relname='$table') and (pg_class.oid=pg_index.indrelid)"] {
		foreach {oid indkey indexrelid indisprimary indisunique} $line break
		set indexname [$db exec "select relname from pg_class where oid=$indexrelid"]
		lappend data(indices) $indexname
		foreach num $indkey {
			incr num -1
			lappend data(index,[lindex $data(fields) $num],name) $indexname
		}
		set data(index,$indexname,isprimary) [? {"$indisprimary" == "t"} 1 0]
		set data(index,$indexname,isunique) [? {"$indisunique" == "t"} 1 0]
		if $data(index,$indexname,isprimary) {
			lappend data(primarykey) $data(index,$indexname,key)
		}
	}
	foreach line [$db exec "select oid,tgname,tgtype,tgargs from pg_trigger where (pg_class.relname='$table') and (pg_class.oid=pg_trigger.tgrelid)"] {
		foreach {oid tgname tgtype tgargs} $line break
		if ![regexp ^RI_ConstraintTrigger_ $tgname] continue
		if {$tgtype == 21} {
			set line [string_split $tgargs \\000]
			set data(foreignkey,[lindex $line 4]) [list [lindex $line 2] [lindex $line 5]]
			lappend data(foreignkeys) [lindex $line 4]
		} elseif {$tgtype == 9} {
			set line [string_split $tgargs \\000]
			lappend data(referencedby,[lindex $line 1]) [lrange $line 4 5]
			lappend data(referencedby) [lindex $line 1]
		}
	}
	foreach line [$db exec "SELECT rcsrc FROM pg_relcheck r, pg_class c WHERE c.relname='$table' AND c.oid = r.rcrelid"] {
		lappend data(constraints) [lindex $line 0]
	}
}
