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
	db open $::testdatabase
	db close
} {}

# Open test database for further tests
db open $::testdatabase

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
	set try {}
} {}

test $what {select} {
	initdb
	db exec {select * from person}
} {{1 Peter {De Rijk}} {2 John Do} {3 Oog {}}}

test $what {select with -nullvalue} {
	initdb
	db exec -nullvalue NULL {select * from person}
} {{1 Peter {De Rijk}} {2 John Do} {3 Oog NULL}}

test $what {error} {
	initdb
	db exec {select try from person}
} {database error while executing command "select try from person":
ERROR:  attribute 'try' not found
} 1

test $what {select fetch} {
	initdb
	db exec -usefetch {select * from person}
} {}

test $what {1 fetch} {
	initdb
	db exec -usefetch {select * from person}
	db fetch
} {1 Peter {De Rijk}}

test $what {2 fetch} {
	initdb
	db exec -usefetch {select * from person}
	db fetch
	db fetch
} {2 John Do}

test $what {3 fetch} {
	initdb
	db exec -usefetch {select * from person}
	db fetch
	db fetch
	db fetch
} {3 Oog {}}

test $what {4 fetch, end} {
	initdb
	db exec -usefetch {select * from person}
	db fetch
	db fetch
	db fetch
	db fetch
} {line 3 out of range} 1

test $what {fetch 1} {
	initdb
	db exec -usefetch {select * from person}
	db fetch 1
} {2 John Do}

test $what {fetch 1 1} {
	initdb
	db exec -usefetch {select * from person}
	db fetch 1 1
} {John}

test $what {fetch with NULL} {
	initdb
	db exec -usefetch {select * from person}
	db fetch 2
} {3 Oog {}}

test $what {fetch with -nullvalue} {
	initdb
	db exec -usefetch {select * from person}
	db fetch -nullvalue NULL 2
} {3 Oog NULL}

test $what {fetch -lines} {
	initdb
	db exec -usefetch {select * from person}
	db fetch -lines
} {3}

test $what {fetch -fields} {
	initdb
	db exec -usefetch {select * from person}
	db fetch -fields
} {id first_name name}

test $what {fetch with no fetch result available} {
	initdb
	catch {db fetch -clear}
	db exec {select * from person}
	db fetch
} {no result available: invoke exec method with -usefetch option first} 1

test $what {fetch -isnull 1} {
	initdb
	db exec -usefetch {select * from person}
	db fetch -isnull 2 2
} 1

test $what {fetch -isnull 0} {
	initdb
	db exec -usefetch {select * from person}
	db fetch -isnull 2 1
} 0

test $what {fetch field out of range} {
	initdb
	db exec -usefetch {select * from person}
	db fetch 2 3
} {field 3 out of range} 1

test $what {fetch line out of range} {
	initdb
	db exec -usefetch {select * from person}
	db fetch 3
} {line 3 out of range} 1

test $what {fetch -isnull field out of range} {
	initdb
	db exec -usefetch {select * from person}
	db fetch -isnull 2 5
} {field 5 out of range} 1

db close

}

testsummarize
