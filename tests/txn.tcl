#!/bin/sh
# the next line restarts using wish \
exec tclsh8.0 "$0" "$@"
puts "source [info script]"

if ![info exists type] {
	set type [lindex $argv 0]
	set argv [lrange $argv 1 end]
}

source tools.tcl
set type postgresql

set db [dbi $::type db]
db open $::testdatabase

if ![catch {$db begin} result] {

test $what {commit} {
	initdb
	db begin
	db exec {
		insert into person (id,first_name, name)
			values ('t1','Test', 'One');
	}
	db exec {
		insert into person (id,first_name, name)
			values ('t2','Test', 'Two');
	}
	db commit
	db exec {select name from person where first_name = 'Test'}
} {One Two}

test $what {rollback} {
	initdb
	db begin
	db exec {
		insert into person (id,first_name, name)
			values ('t1','Test', 'One');
	}
	db exec {
		insert into person (id,first_name, name)
			values ('t2','Test', 'Two');
	}
	db rollback
	db exec {select name from person where first_name = 'Test'}
} {}

test $what {start with commit} {
	initdb
	db commit
	db begin
	db exec {
		insert into person (id,first_name, name)
			values ('t1','Test', 'One');
	}
	db exec {
		insert into person (id,first_name, name)
			values ('t2','Test', 'Two');
	}
	db commit
	db exec {select name from person where first_name = 'Test'}
} {One Two}

test $what {start with commit, and rollback} {
	initdb
	db commit
	db begin
	db exec {
		insert into person (id,first_name, name)
			values ('t1','Test', 'One');
	}
	db exec {
		insert into person (id,first_name, name)
			values ('t2','Test', 'Two');
	}
	db rollback
	db exec {select name from person where first_name = 'Test'}
} {}

test $what {one command with error} {
	initdb
	catch {db exec {
		insert into person (id,first_name, name)
			values ('t1','Test', 'One');
		error;
		insert into person (id,first_name, name)
			values ('t2','Test', 'Two');
	}}
	db exec {select name from person where first_name = 'Test'}
} {}

} else {
	puts "Backend $type does not support transactions"
}

db close

testsummarize
