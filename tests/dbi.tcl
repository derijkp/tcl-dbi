#!/bin/sh
# the next line restarts using wish \
exec tclsh8.0 "$0" "$@"
puts "source [info script]"

if ![info exists type] {
	set type [lindex $argv 0]
	set argv [lrange $argv 1 end]
}
if ![string length $type] {set type postgresql}

source tools.tcl

set what dbi-$type

test $what {error} {
	set db [dbi $::type db]
	db
} {wrong # args: should be "db option ?...?"} 1

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

test $what {open error} {
	set db [dbi $::type db]
	catch {db open xxxxx}
	db exec {select * from test}
} {dbi object has no open database, open a connection first} 1

# Open test database for further tests
db open $::testdatabase

test $what {open -user error} {
	set db [dbi $::type db]
	db open testdbi -user test -password afsg
} {} 1

test $what {create table} {
	cleandb
	createdb
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
	if ![catch {db exec {select try from person}} result] {
		error "test should cause an error"
	}
	regexp {^database error executing command "select try from person":.*attribute 'try' not found} $result
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

test $what {5 fetch, end} {
	initdb
	db exec -usefetch {select * from person}
	db fetch
	db fetch
	db fetch
	catch {db fetch}
	db fetch
} {line 3 out of range} 1

initdb
db exec -usefetch {select * from person}
set fetchnum [supported {db fetch 1}]

if $fetchnum {

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

test $what {fetch -current} {
	initdb
	db exec -usefetch {select * from person}
	db fetch 0
	db fetch -current
} 1

} else {
# no fetch positioning
test $what {fetch 1} {
	initdb
	db exec -usefetch {select * from person}
	set cmd {db fetch 1}
	if ![catch $cmd result] {
		error "test should cause error"
	}
	regexp {positioning for fetch not supported$} $result
} 1

test $what {fetch -lines} {
	initdb
	db exec -usefetch {select * from person}
	set cmd {db fetch -lines}
	if ![catch $cmd result] {
		error "test should cause error"
	}
	regexp {fetch -lines not supported$} $result
} 1

}

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

db close

testsummarize
