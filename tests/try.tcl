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

