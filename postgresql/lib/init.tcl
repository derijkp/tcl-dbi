#
# Load libs
# ---------
#

package require dbi

namespace eval dbi::postgresql {}

# $Format: "set ::dbi::postgresql::version 0.$ProjectMajorVersion$"$
set ::dbi::postgresql::version 0.8
# $Format: "set ::dbi::postgresql::patchlevel $ProjectMinorVersion$"$
set ::dbi::postgresql::patchlevel 9
package provide dbi_postgresql $::dbi::postgresql::version

source $dbi::postgresql::dir/lib/package.tcl
package::init $dbi::postgresql::dir dbi_postgresql

proc ::dbi::postgresql::string_split {string splitstring} {
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

#
# Procs
# -----
#
set ::dbi::postgresql::privatedbnum 1
proc ::dbi::postgresql::privatedb {db} {
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
	set privatedb($parent) ::dbi::postgresql::priv_$privatedbnum
	incr privatedbnum
	$parent clone $privatedb($parent)
	return $privatedb($parent)
}

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

proc ::dbi::postgresql::fieldsinfo {db table} {
	set db [privatedb $db]
	set c [$db exec "\
		select attnum,attname,attnum,typname,attlen,attnotnull,atttypmod,usename,usesysid,pg_class.oid,relpages,reltuples,relhaspkey,relhasrules,relacl \
		from pg_class,pg_user,pg_attribute,pg_type \
		where (pg_class.relname='$table') and (pg_class.oid=pg_attribute.attrelid) and (pg_class.relowner=pg_user.usesysid) and (pg_attribute.atttypid=pg_type.oid) \
		order by attnum"] 
	if ![llength $c] {error "table \"$table\" does not exist"}
	set result ""
	foreach line $c {
		if {[lindex $line 2] > 0} {
			lappend result [lindex $line 1]
		}
	}
	return $result
}

array set ::dbi::postgresql::typetrans {bpchar char int4 integer int2 smallint float8 double}

proc ::dbi::postgresql::serial_add {db table field args} {
	set db [privatedb $db]
	set name ${table}_${field}_seq
	catch {$db exec "drop sequence \"$name\""}
	catch {$db exec "drop index \"${table}_${field}_key\""}
	$db exec [subst -nobackslashes -nocommands {
		create sequence $name;
		alter table $table alter $field set default nextval('$name');
		create unique index ${table}_${field}_key on $table ($field);
	}]
	if [llength $args] {
		set current [lindex $args 0]
		if {$current < 1} {
			error "cannot set serial to value < 1"
		}
		$db exec [subst -nobackslashes -nocommands {
			select setval('$name',$current)
		}]
	} else {
		set current 1
	}
	return $current	
}

proc ::dbi::postgresql::serial_delete {db table field} {
	set db [privatedb $db]
	set name ${table}_${field}_seq
	catch {$db exec "alter table \"$table\" alter \"$field\" drop default"}
	catch {$db exec "drop sequence \"$name\";"}
	catch {$db exec "drop index \"${table}_${field}_key\""}
}

proc ::dbi::postgresql::serial_set {db table field args} {
	set db [privatedb $db]
	set name ${table}_${field}_seq
	if [llength $args] {
		$db exec [subst -nobackslashes {
			select setval('$name',[lindex $args 0]);
		}]
	} else {
		$db exec [subst -nobackslashes {
			select currval('$name') from $name;
		}]
	}
}

proc ::dbi::postgresql::serial_next {db table field} {
	set db [privatedb $db]
	set name ${table}_${field}_seq
	$db exec [subst -nobackslashes -nocommands {
		select nextval('$name');
	}]
}

proc ::dbi::postgresql::table_info_old {db table} {
	upvar ::dbi::postgresql::typetrans typetrans
	set result ""
	set c [$db exec "\
		select oid,relpages,reltuples,relhaspkey,relhasrules,relowner,relacl \
		from pg_class \
		where pg_class.relname='$table'"] 
	foreach {pg_class_oid relpages reltuples relhaspkey relhasrules relowner relacl} [lindex $c 0] break
	set owner [$db exec "select usename from pg_user where usesysid = $relowner"] 
	set c [$db exec "\
		select attnum,attname,typname,attlen,attnotnull,atttypmod \
		from pg_attribute,pg_type \
		where (pg_attribute.attrelid = $pg_class_oid) and (pg_attribute.atttypid=pg_type.oid) \
		order by attnum"] 
	set fields {}
	foreach line $c {
		foreach {attnum attname typname attlen attnotnull atttypmod} $line break
		if {$attnum < 0} continue
		lappend fields $attname
		if [::info exists typetrans($typname)] {
			lappend result type,$attname $typetrans($typname)
		} else {
			lappend result type,$attname $typname
		}
		if {$atttypmod == -1} {
			lappend result length,$attname $attlen
		} else {
			lappend result length,$attname [expr {$atttypmod-4}]
		}
		if {"$attnotnull" == "t"} {
			lappend result notnull,$attname 1
		}
	}
	lappend result fields $fields
	lappend result owner $owner
	lappend result oid ${pg_class_oid}
	lappend result ownerid $relowner
	lappend result numtuples $reltuples
	lappend result numpages $relpages
	lappend result permissions $relacl
	set c [$db exec "select indkey,indexrelid,indisprimary,indisunique from pg_index where (pg_index.indrelid = $pg_class_oid)"]
	foreach line $c {
		foreach {indkey indexrelid indisprimary indisunique} $line break
		set indexname [$db exec "select relname from pg_class where oid=$indexrelid"]
		lappend indices $indexname
		set indkeylist {}
		foreach num $indkey {
			incr num -1
			lappend indkeylist [lindex $fields $num]
		}
		lappend result index,$indkeylist,name $indexname
		if {"$indisprimary" == "t"} {
			lappend result primary,$indkeylist $indexname
		}
		if {"$indisunique" == "t"} {
			lappend result unique,$indkeylist $indexname
		}
	}
	set c [$db exec "select oid,tgname,tgtype,tgargs from pg_trigger where (pg_class.relname='$table') and (pg_class.oid=pg_trigger.tgrelid)"]
	foreach line $c {
		foreach {oid tgname tgtype tgargs} $line break
		if ![regexp ^RI_ConstraintTrigger_ $tgname] continue
		if {$tgtype == 21} {
			set templine [string_split $tgargs \\000]
			lappend result foreign,[lindex $templine 4] [list [lindex $templine 2] [lindex $templine 5]]
		} elseif {$tgtype == 9} {
			set templine [string_split $tgargs \\000]
			lappend keepref(referencedby,[lindex $templine 5]) [list [lindex $templine 1] [lindex $templine 4]]
		}
	}
	eval lappend result [array get keepref]
	set c [$db exec "SELECT rcname,rcsrc FROM pg_relcheck r, pg_class c WHERE c.relname='$table' AND c.oid = r.rcrelid"]
	foreach line $c {
		lappend result constraint,[lindex $line 1] [lindex $line 0]
	}
	return $result
}

proc ::dbi::postgresql::table_info_new {db table} {
	upvar ::dbi::postgresql::typetrans typetrans
	set result ""
	set c [$db exec "\
		select oid,relpages,reltuples,relhaspkey,relhasrules,relowner,relacl \
		from pg_class \
		where pg_class.relname='$table'"] 
	foreach {pg_class_oid relpages reltuples relhaspkey relhasrules relowner relacl} [lindex $c 0] break
	set owner [$db exec "select usename from pg_user where usesysid = $relowner"] 
	set c [$db exec "\
		select attnum,attname,typname,attlen,attnotnull,atttypmod \
		from pg_attribute,pg_type \
		where (pg_attribute.attrelid = $pg_class_oid) and (pg_attribute.atttypid=pg_type.oid) \
		order by attnum"] 
	set fields {}
	foreach line $c {
		foreach {attnum attname typname attlen attnotnull atttypmod} $line break
		if {$attnum < 0} continue
		lappend fields $attname
		if [::info exists typetrans($typname)] {
			lappend result type,$attname $typetrans($typname)
		} else {
			lappend result type,$attname $typname
		}
		if {$atttypmod == -1} {
			lappend result length,$attname $attlen
		} else {
			lappend result length,$attname [expr {$atttypmod-4}]
		}
		if {"$attnotnull" == "t"} {
			lappend result notnull,$attname 1
		}
	}
	lappend result fields $fields
	lappend result owner $owner
	lappend result oid ${pg_class_oid}
	lappend result ownerid $relowner
	lappend result numtuples $reltuples
	lappend result numpages $relpages
	lappend result permissions $relacl
	set c [$db exec "select indkey,indexrelid,indisprimary,indisunique from pg_index where (pg_index.indrelid = $pg_class_oid)"]
	foreach line $c {
		foreach {indkey indexrelid indisprimary indisunique} $line break
		set indexname [$db exec "select relname from pg_class where oid=$indexrelid"]
		lappend indices $indexname
		set indkeylist {}
		foreach num $indkey {
			incr num -1
			lappend indkeylist [lindex $fields $num]
		}
		lappend result index,$indkeylist,name $indexname
		if {"$indisprimary" == "t"} {
			lappend result primary,$indkeylist $indexname
		}
		if {"$indisunique" == "t"} {
			lappend result unique,$indkeylist $indexname
		}
	}
	set c [$db exec "select oid,tgname,tgtype,tgargs from pg_trigger where (pg_class.relname='$table') and (pg_class.oid=pg_trigger.tgrelid)"]
	foreach line $c {
		foreach {oid tgname tgtype tgargs} $line break
		if ![regexp ^RI_ConstraintTrigger_ $tgname] continue
		if {$tgtype == 21} {
			set templine [string_split $tgargs \\000]
			lappend result foreign,[lindex $templine 4] [list [lindex $templine 2] [lindex $templine 5]]
		} elseif {$tgtype == 9} {
			set templine [string_split $tgargs \\000]
			lappend keepref(referencedby,[lindex $templine 5]) [list [lindex $templine 1] [lindex $templine 4]]
		}
	}
	eval lappend result [array get keepref]
	set c [$db exec {SELECT conname,consrc FROM pg_constraint r, pg_class c WHERE c.relname=? AND c.oid = r.conrelid} $table]
	foreach line $c {
		lappend result constraint,[lindex $line 1] [lindex $line 0]
	}
	return $result
}

proc ::dbi::postgresql::info {db args} {
	set db [privatedb $db]
	set len [llength $args]
	if {$len < 1} {error "wrong # args: should be \"$db info option ...\""}
	set type [lindex $args 0]
	switch -exact $type {
		user {
			set result [$db exec {select user}]
		}
		fields {
			if {$len == 2} {
				return [$db fields [lindex $args 1]]
			} else {
				error "wrong # args: should be \"$db info fields ?table?\""
			}
		}
		tables {
			return [$db tables]
		}
		systemtables {
			set result [$db exec {select relname from pg_class where (relkind = 'r' or relkind = 'v') and relname like 'pg\\_%'}]
		}
		views {
			set result [$db exec {select viewname from pg_views where (viewname !~ '^pg_' and viewname !~ '^pga_')}]
		}
		access {
			set option [lindex $args 1]
			switch $option {
				select {set char r}
				insert {set char a}
				delete {set char w}
				update {set char w}
				rule {set char R}
				default {error "wrong option \"$option\": must be one of select, insert, delete, update or rule"}
			}
			if {$len == 3} {
				set user [lindex $args 2]
				set userid [$db exec "select usesysid from pg_user where usename = '$user'"]
				set result {}
				set c [$db exec "select relname from pg_class	where relowner = $userid and (relkind = 'r' or relkind = 'v') and relname !~ 'pg_' and relname !~ '^pga_'"]
				foreach table $c {
					lappend result $table
				}
				set c [$db exec "select relname,relacl from pg_class	where relowner != $userid and (relkind = 'r' or relkind = 'v') and relname !~ '^pg_' and relname !~ '^pga_'"]
				foreach line $c {
					foreach {table acl} $line break
					if {[regexp [subst {"($user|)=.*$char.*"}] $acl]} {
						lappend result $table
					}
				}
				set result [lsort $result]
			} elseif {$len == 4} {
				set user [string toupper [lindex $args 2]]
				set userid [$db exec "select usesysid from pg_user where usename = '$user'"]
				set table [lindex $args 3]
				set line [$db exec "select relowner,relacl from pg_class	where relname = '$table'"]
				foreach {owner acl} [lindex $line 0] break
				if {[string equal $user $owner]} {
					return [$db fields $table]
				} elseif {[regexp [subst {"($user|)=.*$char.*"}] $acl]} {
					return [$db fields $table]
				} else {
					return {}
				}
			} else {
				error "wrong # args: should be \"$db info access select user/role ?table?\""
			}
		}
		table {
			if {$len == 2} {
				variable dbtype
				if {![::info exists dbtype($db)]} {
					if {[lsearch [$db info systemtables] pg_relcheck] == -1} {
						set dbtype($db) new
					} else {
						set dbtype($db) old
					}
				}
				set table [lindex $args 1]
				set result [::dbi::postgresql::table_info_$dbtype($db) $db $table]
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
