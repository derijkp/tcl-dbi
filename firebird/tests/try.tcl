package require dbi
package require dbi_firebird
dbi_firebird db
db open localhost:/home/ib/testdbi.gdb -user test -password blabla
db tables
catch {db exec {drop table "person"}}
db exec {
		create table "person" (
			"id" char(6) not null primary key,
			"first_name" varchar(100),
			"name" varchar(100),
			"score" double precision
		);
}
db exec {insert into "person" ("id","name") values ('t','test')}
db exec {select * from "person"}


create database 'localhost:/home/ib/testdbi.fdb';
create table "use" (
	"id" integer not null primary key,
	"score" float,
	"score2" float,
	constraint use_htest check ("score2" > "score")
);
alter table "use" drop constraint use_htest;
drop table "use";
drop database 'localhost:/home/ib/testdbi.fdb';
