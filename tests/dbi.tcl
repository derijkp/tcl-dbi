#!/bin/sh
# the next line restarts using wish \
exec tclsh8.0 "$0" "$@"
#exec ../src/dbtcl "$0" "$@"
puts "source [info script]"

source tools.tcl

test $what {types} {
	dbi types
} {postgresql}

foreach init {
	{
		set ::type postgresql
		set what dbi-$type
	}
} {
eval $init

test $what {create and destroy dbi object} {
	set db [dbi $::type]
	$db destroy
} {}

test $what {create and db error} {
	set db [dbi $::type db]
	db xx
} {bad option "xx": must be one of open, configure, exec, fetch, close} 1

test $what {create and destroy dbi object with name} {
	set db [dbi $::type db]
	db destroy
} {}

# Create dbi object
dbi $::type db

test $what {open and close} {
	db open testdb
	db close
} {}

# Open test database for further tests
db open testdb

test $what {create table} {
	cleandb
	db exec {
		create table test (
			id serial primary key,
			first_name text,
			name text
		);
	}
} {}

test $what {create and fill table} {
	cleandb
	createdb
	filldb
} {}

test $what {select} {
	initdb
	db exec {select * from person}
} {{1 Peter {De Rijk}} {2 John Do}}

test $what {select fetch} {
	initdb
	db exec -fetch {select * from person}
} {2}

test $what {fetch 1} {
	initdb
	db exec -fetch {select * from person}
	db fetch
} {1 Peter {De Rijk}}

test $what {fetch 2} {
	initdb
	db exec -fetch {select * from person}
	db fetch
	db fetch
} {2 John Do}

test $what {fetch 3} {
	initdb
	db exec -fetch {select * from person}
	db fetch
	db fetch
	db fetch
} {}

test $what {error} {
	initdb
	db exec {select try from person}
} {ERROR:  attribute 'try' not found} 1

db close

}

testsummarize
