#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

package require interface
namespace eval interface {}
set type postgresql
set testdb testdbi
set type sqlite
set testdb test.db
set type interbase
set testdb localhost:/home/ib/testdbi.gdb
set type odbc
set testdb testdbi
set type mysql
set testdb test

set interface dbi/try
set version 0.1
set name dbi-01
package require dbi
package require dbi_$type


set interface::testleak 0
set object [dbi_$type]
set object2 [dbi_$type]
array set opt [subst {
	-columnperm 0
	-testdb test
	-openargs {-user test}
	-user2 peter
	-object2 $object2
}]

dbi::setup
dbi::opendb
dbi::initdb

$object tables

set error ""
set field ts
set value "2000-11-18 10:40:30.000"
$object exec {delete from "types"}
$object exec "insert into \"types\"(\"$field\") values(?)" $value
set rvalue [lindex [lindex [$object exec "select \"$field\" from \"types\" where \"$field\" = ?" $value] 0] 0]
if {"$value" != "$rvalue"} {
	append error "different values \"$value\" and \"$rvalue\" for $field\n"
}

interface::test {tables 2} {
	# there maybe other tables than these present
	array set a {address 1 location 1 person 1 types 1 useof 1 v_test 1}
	set tables {}
	foreach table [$object tables] {
		if {[info exists a($table)]} {lappend tables $table}
	}
	lsort $tables
} {address location person types useof*} match

interface::test {special table} {
	catch {$object exec {create table "o$test" (i integer)}}
	set result [lsort [$object tables]]
	$object exec {drop table "o$test"}
	set result
} {address location multi {o$test} person types useof*} match

interface::test {db fields} {
	$object fields person
} {id first_name name score}

#puts "tests done"
#$object close
interface::testsummarize
