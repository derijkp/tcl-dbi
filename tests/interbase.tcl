#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

set user1 peter
set user2 pdr
set type interbase
source tools.tcl
set db [dbi $::type db]
puts "open /home/ib/testdbi.gdb"
db open /home/ib/testdbi.gdb
catch {db exec {drop view v_test;}} result
catch {db exec {drop table duse;}} result
catch {db exec {drop table use;}} result
catch {db exec {drop table test;}} result
catch {db exec {drop table types}} result
catch {db exec {drop table person}} result
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
} {ID FIRST_NAME NAME SCORE}

test select {fetch -lines} {
	db exec -usefetch {select * from test}
	catch {db fetch -lines}
	db fetch
	db fetch
} {2 John Doe 17.5}

test select {fetch and begin/rollback} {
	db exec -usefetch {select * from test}
	db fetch
	db begin
	db exec {
		insert into test(id,first_name,name) values(10,'Try','It');
	}
	catch {db fetch} result
	set result
} {no result available: invoke exec method with -usefetch option first}

test tables {tables} {
	lsort [db tables]
} {TEST TYPES USE}

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
	db tableinfo USE data
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
	db exec {delete from TYPES}
	catch {db serial delete TYPES I}
	db serial add TYPES I
	db exec {insert into types (d) values (?)} 20
	db exec {insert into types (d) values (?)} 21
	db exec {select i,d from types order by d}
} {{1 20.0} {2 21.0}}

test serial {set} {
	db exec {delete from types}
	catch {db serial delete TYPES I}
	db serial add TYPES I 1
	db exec {insert into types (d) values (?)} 20
	db serial set TYPES I 8
	set i [db serial set TYPES I]
	db exec {insert into types (d) values (?)} 21
	list $i [db exec {select i,d from types order by d}]
} {8 {{2 20.0} {9 21.0}}}

test serial {overrule} {
	db exec {delete from types}
	catch {db serial delete TYPES I}
	db serial add TYPES I 1
	db exec {insert into types (d) values (?)} 20
	db exec {insert into types (i,d) values (9,?)} 21
	db exec {select i,d from types order by d}
} {{2 20.0} {9 21.0}}

test serial {next} {
	db exec {delete from types}
	catch {db serial delete TYPES I}
	db serial add TYPES I
	db exec {insert into types (d) values (?)} 20
	set i [db serial next TYPES I]
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

catch {db exec {create role test}}
catch {db exec {create role try}}

test info {roles} {
	db info roles
} {TEST TRY}

catch {db exec "grant test to [db info user]"}
catch {db exec "grant try to [db info user]"}

test info {roles} {
	db info roles peter
} {TEST TRY}

db exec "revoke try from [db info user]"

test info {roles} {
	db info roles peter
} {TEST}

db exec {create view v_test as select ID, FIRST_NAME, NAME from test}

test info {views} {
	db info views
} {V_TEST}

catch {db exec "create domain tdom as varchar(6)"}
catch {db exec "create domain idom as integer"}

test info {domains} {
	db info domains
} {IDOM TDOM}

test info {domain} {
	db info domain TDOM
} {varchar(6) nullable}

test info {table} {
	catch {db exec {drop table duse}}
	db exec {
		create table duse (
			id integer not null primary key,
			td tdom,
			person integer not null unique references test(id),
			place varchar(100),
			usetime timestamp,
			score float check (score < 20.0),
			score2 float check (score2 < 20.0),
			b blob sub_type 1 segment size 20,
			check (score2 > score)
		)
	}
	db tableinfo DUSE t
	array set a [db info table DUSE]
	list $a(fields) $a(type,ID) $a(length,PLACE) [array names a primary,*]
} {{ID TD PERSON PLACE USETIME SCORE SCORE2 B} integer 100 primary,ID}


test bugfix {crash} {
catch {db exec {drop table person}}
db exec {
create table person (
        "id" varchar(6) not null primary key,
        "first_name" varchar(25) ,
        "last_name" varchar(50) ,
        "group" varchar(4) )
}
db exec {
insert into person ("id","first_name","last_name","group") values ('peter','Peter','','binf');
}
set a 1
} 1

test info {access select} {
	db exec {
		grant select on test to pdr
	}
	list [db info access select PDR] [db info access select PETER]
} {TEST {DUSE PERSON TEST TYPES USE V_TEST}}

test info {access select table} {
	db exec {
		grant select on test to pdr
	}
	list [db info access select PDR TEST] [db info access select PETER TEST]
} {{ID FIRST_NAME NAME SCORE} {ID FIRST_NAME NAME SCORE}}

test info {access insert} {
	db exec {
		grant insert on test to pdr
	}
	list [db info access insert PDR] [db info access insert PETER]
} {TEST {DUSE PERSON TEST TYPES USE V_TEST}}


test info {access update} {
	db exec {
		grant update(ID,FIRST_NAME) on test to pdr
	}
	list [db info access update PDR] [db info access update PETER]
} {TEST {DUSE PERSON TEST TYPES USE V_TEST}}

test info {access select table} {
	db exec {
		grant update(ID,FIRST_NAME) on test to pdr
	}
	list [db info access select PDR DUSE] [db info access select PDR TEST] [db info access select PETER TEST]
} {{} {ID FIRST_NAME NAME SCORE} {ID FIRST_NAME NAME SCORE}}

test info {access update table} {
	db exec {
		grant update(ID,FIRST_NAME) on test to pdr
	}
	list [db info access update PDR TEST] [db info access update PETER TEST]
} {{ID FIRST_NAME} {ID FIRST_NAME NAME SCORE}}

test info {error db fields} {
	db fields notexist
} {}
