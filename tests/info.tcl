#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
#exec ../src/dbtcl "$0" "$@"
puts "source [info script]"

if ![info exists type] {
	set type [lindex $argv 0]
	set argv [lrange $argv 1 end]
	if ![string length $type] {set type postgresql}
}

source tools.tcl

set what dbi-$type

dbi $::type db
db open $::testdatabase
cleandb
createdb
filldb

test $what {tables} {
	lsort [db tables]
} {address location person}

test $what {fields} {
	db fields location
	array get test
} {constraints {{((("type" = 'home'::text) OR ("type" = 'work'::text)) OR ("type" = 'leisure'::text))}} field,inhabitant,type char ownerid 27 foreignkey,inhabitant {person id} field,inhabitant,size -1 field,inhabitant,notnull 0 field,address,type int4 oid 34224 field,address,size 4 field,address,notnull 0 field,inhabitant,fsize 10 numpages 10 field,type,type text field,type,size -1 field,type,notnull 0 field,address,fsize -1 owner peter permissions {} foreignkey,address {address id} numtuples 1000 foreignkeys {inhabitant address} hasrules 0 fields {type inhabitant address} field,type,fsize -1}

test $what {tableinfo} {
	db tableinfo location test
	array get test
} {constraints {{((("type" = 'home'::text) OR ("type" = 'work'::text)) OR ("type" = 'leisure'::text))}} field,inhabitant,type char ownerid 27 foreignkey,inhabitant {person id} field,inhabitant,size -1 field,inhabitant,notnull 0 field,address,type int4 oid 34224 field,address,size 4 field,address,notnull 0 field,inhabitant,fsize 10 numpages 10 field,type,type text field,type,size -1 field,type,notnull 0 field,address,fsize -1 owner peter permissions {} foreignkey,address {address id} numtuples 1000 foreignkeys {inhabitant address} hasrules 0 fields {type inhabitant address} field,type,fsize -1}

test $what {tableinfo on non existing table} {
	db tableinfo doesnotexist test
} {table "doesnotexist" does not exist} 1

test $what {tableinfo error in format} {
	db tableinfo location
} {wrong # args: should be "db tableinfo tablename varName"} 1

test $what {tableinfo empty var} {
	db tableinfo location {}
	set (fields)
} {type inhabitant address}

if $fts(datasources) {

test $what {datasources} {
	expr {[lsearch [db info datasources] testdbi] != -1}
} {1}

}

db close

testsummarize
