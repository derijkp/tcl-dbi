#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

set type interbase
source tools.tcl
set db [dbi $::type db]
puts "open /home/ib/testdbi.gdb"
db open /home/ib/testdbi.gdb
catch {db exec {drop table use;}} result
catch {db exec {drop table test;}} result
catch {db exec {drop table types}} result
db exec {
	create table test (
		id integer not null primary key,
		first_name varchar(100),
		name varchar(100),
		score double precision
	);
}
db exec {
	create table use (
		id integer not null primary key,
		person integer not null unique references test(id),
		place varchar(100),
		usetime timestamp,
		score float check (score < 20.0),
		score2 float check (score < 20.0),
		b blob sub_type 1 segment size 20,
		check (score2 > score)
	);
	create index use_score_idx on use(score);
	create table types (
		i integer,
		si smallint,
		vc varchar(10),
		c char(10),
		f float,
		d double precision,
		da date,
		t time,
		ts timestamp,
		b blob
	);
}
puts inserting
db exec {
	insert into test values(1,'Peter','De Rijk',20);
}
db exec {
	insert into test values(2,'John','Doe',17.5);
	insert into test (id,first_name) values(3,'Jane');
}

puts selects
test select {basic} {
	db exec {select * from test}
} {{1 Peter {De Rijk} 20.0} {2 John Doe 17.5} {3 Jane {} {}}}

test select {again} {
	db exec {select * from test}
} {{1 Peter {De Rijk} 20.0} {2 John Doe 17.5} {3 Jane {} {}}}

test select {fetch} {
	db exec -usefetch {select * from test}
	db fetch
	db fetch
} {2 John Doe 17.5}

test select {fetch 2} {
	db exec -usefetch {select * from test}
	db fetch 2
} {2 John Doe 17.5}

test select {fetch 2 1} {
	db exec -usefetch {select * from test}
	db fetch 2 1
} {John}

test select {fetch error} {
	db exec -usefetch {select * from test}
	db fetch 2 1
	db fetch 1
} {interbase error: backwards positioning for fetch not supported} 1

test select {fetch error} {
	db exec -usefetch {select * from test}
	db fetch 2
	db fetch 2
} {2 John Doe 17.5}

test select {fetch -nullvalue} {
	db exec -usefetch {select * from test}
	db fetch -nullvalue null 3
} {3 Jane null null}

test select {fetch -isnull} {
	db exec -usefetch {select * from test}
	db fetch -isnull 3 2
} {1}

test select {fetch -isnull} {
	db exec -usefetch {select * from test}
	db fetch -isnull 3 0
} {0}

test select {fetch -fields} {
	db exec -usefetch {select * from test}
	db fetch -fields
} {id first_name name score}

test select {fetch -lines} {
	db exec -usefetch {select * from test}
	catch {db fetch -lines}
	db fetch
	db fetch
} {2 John Doe 17.5}

test tables {tables} {
	lsort [db tables]
} {test types use}

test types {types} {
	set error ""
	foreach {field value} [list i 20 si 10 vc test c test f 18.5 d 19.5 da "2000-11-18" t 10:40:30.000 ts "2000-11-18 10:40:30.000"] {
		# puts [list $field $value]
		db exec {delete from types}
		db exec "insert into types($field) values(?)" $value
		set rvalue [lindex [lindex [db exec "select $field from types where $field = ?" $value] 0] 0]
		if {"$value" != "$rvalue"} {
			append error "different values \"$value\" and \"$rvalue\" for $field\n"
		}
		# puts [list $field $rvalue]
	}
	set value {};for {set i 1} {$i < 200} {incr i} {lappend value $i}
	set field b
	# puts [list $field $value]
	db exec {delete from types}
	db exec "insert into types($field) values(?)" $value
	set rvalue [lindex [lindex [db exec "select $field from types"] 0] 0]
	if {"$value" != "$rvalue"} {
		append error "different values \"$value\" and \"$rvalue\" for $field\n"
	}
	# puts [list $field $rvalue]
	if [string length $error] {error $error}
	set a 1
} 1

test select {table info} {
	db tableinfo use data
	set result ""
	foreach {k v} [array get data check*] {lappend result $v}
	lappend result [array names data index,*,name]
	foreach {k v} [array get data field*] {lappend result [list $k $v]}
	lsort $result
} {{check (score < 20.0)} {check (score < 20.0)} {check (score2 > score)} {field,,notnull 1} {field,b,ftype blob} {field,b,notnull 0} {field,b,size 8} {field,b,type blob} {field,id,ftype integer} {field,id,notnull 1} {field,id,primarykey 1} {field,id,size 4} {field,id,type integer} {field,person,ftype integer} {field,person,notnull 1} {field,person,size 4} {field,person,type integer} {field,person,unique 1} {field,place,ftype varchar(100)} {field,place,notnull 0} {field,place,size 100} {field,place,type varchar} {field,score,ftype float} {field,score,notnull 0} {field,score,size 4} {field,score,type float} {field,score2,ftype float} {field,score2,notnull 0} {field,score2,size 4} {field,score2,type float} {field,usetime,ftype timestamp} {field,usetime,notnull 0} {field,usetime,size 8} {field,usetime,type timestamp} {fields {id person place usetime score score2 b}} {index,score,name index,id,name index,person,name}}

test transactions {transactions} {
	db exec {delete from test;}
	set r1 [db exec {select first_name from test order by id;}]
	db begin
	db exec {
		insert into test values(1,'Peter','De Rijk',20);
	}
	db exec {
		insert into test values(2,'John','Doe',17.5);
		insert into test (id,first_name) values(3,'Jane');
	}
	set r2 [db exec {select first_name from test order by id;}]
	db rollback
	set r3 [db exec {select first_name from test order by id;}]
	db begin
	db exec {
		insert into test values(1,'Peter','De Rijk',20);
	}
	db exec {
		insert into test values(2,'John','Doe',17.5);
		insert into test (id,first_name) values(3,'Jane');
	}
	db commit
	set r4 [db exec {select first_name from test order by id;}]
	list $r1 $r2 $r3 $r4
} {{} {Peter John Jane} {} {Peter John Jane}}

test transactions {transactions via exec} {
	db exec {delete from test;}
	set r1 [db exec {select first_name from test order by id;}]
	db exec {set transaction}
	db exec {
		insert into test values(1,'Peter','De Rijk',20);
	}
	db exec {
		insert into test values(2,'John','Doe',17.5);
		insert into test (id,first_name) values(3,'Jane');
	}
	set r2 [db exec {select first_name from test order by id;}]
	db exec rollback
	set r3 [db exec {select first_name from test order by id;}]
	db exec {set transaction}
	db exec {
		insert into test values(1,'Peter','De Rijk',20);
	}
	db exec {
		insert into test values(2,'John','Doe',17.5);
		insert into test (id,first_name) values(3,'Jane');
	}
	db exec commit
	set r4 [db exec {select first_name from test order by id;}]
	list $r1 $r2 $r3 $r4
} {{} {Peter John Jane} {} {Peter John Jane}}

test transactions {autocommit error} {
	db exec {delete from test;}
	set r1 [db exec {select first_name from test;}]
	catch {db exec {
		insert into test values(1,'Peter','De Rijk',20);
		insert into test values(2,'John','Doe','error');
		insert into test (id,first_name) values(3,'Jane');
	}}
	list $r1 [db exec {select first_name from test;}]
} {{} {}}

test transactions {begin error} {
	db exec {delete from test;}
	set r1 [db exec {select first_name from test;}]
	db begin
	catch {db exec {
		insert into test values(1,'Peter','De Rijk',20);
		insert into test values(2,'John','Doe','error');
		insert into test (id,first_name) values(3,'Jane');
	}}
	set r2 [db exec {select first_name from test;}]
	db rollback
	set r3 [db exec {select first_name from test;}]
	list $r1 $r2 $r3
} {{} Peter {}}

test serial {basic} {
	db exec {delete from types}
	catch {db serial delete types i}
	db serial add types i
	db exec {insert into types (d) values (?)} 20
	db exec {insert into types (d) values (?)} 21
	db exec {select i,d from types order by d}
} {{1 20.0} {2 21.0}}

test serial {set} {
	db exec {delete from types}
	catch {db serial delete types i}
	db serial add types i 1
	db exec {insert into types (d) values (?)} 20
	db serial set types i 8
	set i [db serial set types i]
	db exec {insert into types (d) values (?)} 21
	list $i [db exec {select i,d from types order by d}]
} {8 {{2 20.0} {9 21.0}}}

test serial {overrule} {
	db exec {delete from types}
	catch {db serial delete types i}
	db serial add types i 1
	db exec {insert into types (d) values (?)} 20
	db exec {insert into types (i,d) values (9,?)} 21
	db exec {select i,d from types order by d}
} {{2 20.0} {9 21.0}}

test serial {next} {
	db exec {delete from types}
	catch {db serial delete types i}
	db serial add types i
	db exec {insert into types (d) values (?)} 20
	set i [db serial next types i]
	db exec {insert into types (d) values (?)} 20
	list $i [db exec {select i from types order by d}]
} {2 {1 3}}

test parameters {error} {
	db exec {select * from test where name = ?}
} {wrong number of arguments given to exec while executing command: "select * from test where name = ?"} 1

test database {create} {
	catch {db close}
	catch {
		db open /home/ib/try.gdb -user pdr -password pdr
		db drop
	}
	db create /home/ib/try.gdb -user pdr -password pdr
	db open /home/ib/try.gdb -user pdr -password pdr
	db tables
} {}

test database {create and drop} {
	catch {db close}
	catch {
		db open /home/ib/try.gdb -user pdr -password pdr
		db drop
	}
	db create /home/ib/try.gdb -user pdr -password pdr
	db open /home/ib/try.gdb -user pdr -password pdr
	db drop
	db exec { }
} {dbi object has no open database, open a connection first} 1

test database {trigger with declare} {
	db open /home/ib/testdbi.gdb
	catch {db exec {drop trigger test_Update}}
	db exec {
	create trigger test_Update for test before update as
	declare variable current_project varchar(8);
	begin
		update test
		set name = NEW.name
		where id = OLD.id;
	end
	}
	db exec {update test set first_name = 'test' where id = 3}
} {}

