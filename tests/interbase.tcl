#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

set type interbase
source tools.tcl
set db [dbi $::type db]
puts "open /home/ib/test.gdb"
db open /home/ib/test.gdb -user pdr -password pdr
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

test select {fetch -isnull} {
	db exec -usefetch {select * from test}
	db fetch -fields
} {id first_name name score}

test tables {tables} {
	db tables
} {test use types}

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

test generator {generator} {
	catch {db exec {
		delete from rdb$generators where rdb$generator_name = 'TEST_ID_SEQ';
		drop trigger test_id_seq;
	}}
	catch {db exec {
		delete from rdb$generators where rdb$generator_name = 'USE_ID_SEQ';
		drop trigger use_id_seq;
	}}
	db exec {
		create generator test_id_seq;
		create trigger test_id_seq for test before
		insert as
		begin
			new.id = cast (gen_id(test_id_seq,1) as integer);
		end;
		set generator test_id_seq to 3
	}
	db exec {
		create generator use_id_seq;
		create trigger use_id_seq for use before
		insert as
		begin
			new.id = cast (gen_id(use_id_seq,1) as integer);
		end;
		set generator use_id_seq to 3
	}
	db exec {
		insert into test (first_name) values('Oog');
	}
	db exec {select * from test where first_name = 'Oog'}
} {{4 Oog {} {}}}

test select {table info} {
	db tableinfo use data
	array get data
} {{index,rdb$foreign174,isforeign} 1 {index,rdb$173,isunique} 1 field,usetime,type timestamp field,usetime,size 8 field,id,type integer field,id,size 4 field,usetime,notnull 0 index,score,name use_score_idx {index,rdb$primary172,isunique} 1 field,place,notnull 0 foreignkey,person {test id} index,use_score_idx,isunique 0 field,person,notnull 1 index,person,name {rdb$foreign174} {index,rdb$primary172,isforeign} 0 field,,notnull 1 field,b,type blob field,b,size 8 index,use_score_idx,isforeign 0 check,INTEG_334 {check (score < 20.0)} field,person,unique 1 field,b,notnull 0 field,score,type float check,INTEG_335 {check (score < 20.0)} field,score,size 4 owner pdr field,place,type varchar check,INTEG_336 {check (score2 > score)} field,score2,type float field,score,notnull 0 field,place,size 100 {index,rdb$foreign174,isunique} 0 {index,rdb$173,isforeign} 0 field,score2,size 4 field,id,notnull 1 field,person,type integer index,id,name {rdb$primary172} field,id,primarykey 1 field,score2,notnull 0 field,person,size 4 fields {id person place usetime score score2 b}}

test transactions {transactions} {
	db exec {delete from test;}
	set r1 [db exec {select first_name from test;}]
	db begin
	db exec {
		insert into test values(1,'Peter','De Rijk',20);
	}
	db exec {
		insert into test values(2,'John','Doe',17.5);
		insert into test (id,first_name) values(3,'Jane');
	}
	set r2 [db exec {select first_name from test;}]
	db rollback
	set r3 [db exec {select first_name from test;}]
	db begin
	db exec {
		insert into test values(1,'Peter','De Rijk',20);
	}
	db exec {
		insert into test values(2,'John','Doe',17.5);
		insert into test (id,first_name) values(3,'Jane');
	}
	db commit
	set r4 [db exec {select first_name from test;}]
	list $r1 $r2 $r3 $r4
} {{} {Peter John Jane} {} {Peter John Jane}}

test transactions {transactions via exec} {
	db exec {delete from test;}
	set r1 [db exec {select first_name from test;}]
	db exec {set transaction}
	db exec {
		insert into test values(1,'Peter','De Rijk',20);
	}
	db exec {
		insert into test values(2,'John','Doe',17.5);
		insert into test (id,first_name) values(3,'Jane');
	}
	set r2 [db exec {select first_name from test;}]
	db exec rollback
	set r3 [db exec {select first_name from test;}]
	db exec {set transaction}
	db exec {
		insert into test values(1,'Peter','De Rijk',20);
	}
	db exec {
		insert into test values(2,'John','Doe',17.5);
		insert into test (id,first_name) values(3,'Jane');
	}
	db exec commit
	set r4 [db exec {select first_name from test;}]
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

