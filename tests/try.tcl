package require ClassyTk
source tools.tcl

if 0 {
	TblEditor .edit
	Classy::Builder .classy__.builder
	set w [tbledit db TEST]
}
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

