package require dbi
catch {tk appname test}

#set testleak 50
set what test

set testdatabase testdbi

proc cleandb {} {
	catch {db exec "drop table test"}
	catch {db exec "drop sequence test_id_seq"}
	catch {db exec "drop table person"}
	catch {db exec "drop sequence person_id_seq"}
	catch {db exec "drop table address"}
	catch {db exec "drop sequence address_id_seq"}
	catch {db exec "drop table location"}
}

proc createdb {} {
	db exec {
		create table person (
			id serial primary key,
			first_name text,
			name text
		);
		create table address (
			id serial primary key,
			street text,
			number text,
			code text,
			city text
		);
		create table location (
			type text,
			person int4,
			address int4
		);
	}
}

proc filldb {} {
	db exec {
		insert into person (first_name, name)
			values ('Peter', 'De Rijk');
		insert into person (first_name, name)
			values ('John', 'Do');
		insert into person (first_name)
			values ('Oog');
		insert into address (street, number, code, city)
			values ('Universiteitsplein', '1', '2610', 'Wilrijk');
		insert into address (street, number, code, city)
			values ('Melkweg', '10', '1', 'Heelal');
		insert into address (street, number, code)
			values ('Road', '0', '???');
		insert into location (type, person, address)
			select 'work', person.id, address.id from person, address
			where name = 'De Rijk' and street = 'Universiteitsplein';
		insert into location (type, person, address)
			select 'home', person.id, address.id from person, address
			where name = 'De Rijk' and street = 'Melkweg';
		insert into location (type, person, address)
			select 'home', person.id, address.id from person, address
			where name = 'Do' and street = 'Road';
	}
}

proc initdb {} {
	cleandb
	createdb
	filldb
}

if ![info exists testleak] {
	if {"$argv" != ""} {
		set testleak [lindex $argv 0]
	} else {
		set testleak 0
	}
}

proc putsvars args {
	foreach arg $args {
		puts [list set $arg [uplevel set $arg]]
	}
	puts "\n"
}

proc testerror {cmd expected} {
	if ![catch {uplevel $cmd} result] {
		error "cmd should cause an error"
	}
	if {"$result" != "$expected"} {
		error "error message is \"$result\"\nshould be \"$expected\""
	}
	return 1
}

proc testleak {name description script expected causeerror take} {
	global errors testleak
	set line1 [lindex [split [exec ps l [pid]] "\n"] 1]
	time {set error [catch tools__try result]} $testleak
	set line2 [lindex [split [exec ps l [pid]] "\n"] 1]
	if {([lindex $line1 6] != [lindex $line2 6])||([lindex $line1 7] != [lindex $line2 7])} {
		puts "\ntake $take: possible leak:"
		puts $line1
		puts $line2
		incr take
		if {$take == 4} return
		testleak $name $description $script $expected $causeerror $take
	} elseif {$take != 1} {
		puts "take $take ok\n"
	}
}

proc test {name description script expected {causeerror 0} args} {
	global errors testleak
	puts "testing $name: $description"
	proc tools__try {} $script
	set error [catch tools__try result]
	if $causeerror {
		if !$error {
			puts "test should cause an error\nresult is \n$result"
			lappend errors "$name:$description" "test should cause an error\nresult is \n$result"
			return
		}	
	} else {
		if $error {
			puts "test caused an error\nerror is \n$result\n"
			lappend errors "$name:$description" "test caused an error\nerror is \n$result\n"
			return
		}
	}
	if {"$result"!="$expected"} {
		puts "error: result is:\n$result\nshould be\n$expected"
		lappend errors "$name:$description" "error: result is:\n$result\nshould be\n$expected"
	}
	if $testleak {
		if {"$args" != "noleak"} {testleak $name $description $script $expected $causeerror 1}
	}
	return
}


proc testsummarize {} {
global errors
if [info exists errors] {
	set error "***********************\nThere were errors in the tests"
	foreach {test err} $errors {
		append error "\n$test  ----------------------------"
		append error "\n$err"
	}
	error $error
} else {
	puts "All tests ok"
}
}

catch {unset errors}

if $testleak {
	test test {initialise all memory for testing with leak detection} {
		set try 1
	} 1 0 noleak
}
