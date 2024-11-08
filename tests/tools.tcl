if {![::info exists opt(-openargs)]} {set opt(-openargs) {}}

namespace eval dbi {}

proc ::dbi::cleandb {} {
	upvar object object
	upvar opt opt
	catch {$object rollback}
	set fresult ""
	catch {$object serial delete test id} result
	append fresult $result\n
	catch {$object serial delete address id} result
	append fresult $result\n
	if {[$object supports views]} {
		catch {$object exec {drop view "v_test"}} result
		append fresult $result\n
	}
	catch {$object exec {
		alter table "use" drop constraint use_htest
	}} result
	foreach table {duse use test types location address person bl multi t} {
		catch {$object exec [subst {delete from "$table"}]} result
		catch {$object exec [subst {drop table "$table"}]} result
		append fresult $result\n
	}
	return $fresult
}

proc ::dbi::ifsupp {feature string} {
	upvar object object
	if {[$object supports $feature]} {
		return $string
	} else {
		return ""
	}
}

proc ::dbi::createdb {} {
	upvar object object
	upvar opt opt
	$object exec {
		create table "person" (
			"id" char(6) not null primary key,
			"first_name" varchar(100),
			"name" varchar(100),
			"score" double precision
		);
		create table "address" (
			"id" int not null primary key,
			"street" varchar(100),
			"number" varchar(20),
			"code" varchar(10),
			"city" varchar(100)
		);
	}
	$object exec [subst {
		create table "location" (
			"type" varchar(100)[ifsupp check { check ("type" in ('home','work','leisure'))}],
			"inhabitant" char(6)[ifsupp foreignkeys { references "person"("id")}],
			"address" integer[ifsupp foreignkeys { references "address"("id")}]
		);
	}]
	$object exec [subst {
		create table "use" (
			"id" integer not null primary key,
			"person" integer not null unique[ifsupp foreignkeys { references "person"("id")}],
			"place" varchar(100),
			"usetime" timestamp,
			"score" float[ifsupp check { check ("score" < 20.0)}],
			"score2" float[ifsupp check { check ("score" < 20.0),
			constraint use_htest check ("score2" > "score")}]
		);
	}]
	$object exec {select "id" from "use"}
	catch {$object exec {
		create index "use_score_idx" on "use"("score")
	}}
	$object exec {select "id" from "use"}
	$object exec {
		create table "types" (
			"i" integer,
			"si" smallint,
			"vc" varchar(10),
			"c" char(10),
			"f" float,
			"d" double precision,
			"da" date,
			"t" time,
			"ts" timestamp
		);
	}
	$object exec {
		create table "multi" (
			"i" integer not null,
			"si" smallint not null,
			"vc" varchar(10) not null,
			"c" char(10),
			"f" float,
			"d" double precision,
			"da" date,
			"t" time,
			"ts" timestamp,
			primary key("i","si"),
			unique("i","si","vc")
		);
	}
	if {[$object supports views]} {
		$object exec {create view "v_test" as select "id", "first_name", "name" from "person"}
	}
	catch {$object serial add address id}
}

proc ::dbi::filldb {} {
	upvar object object
	upvar opt opt
	$object exec {
		insert into "person" ("id","first_name","name","score")
			values ('pdr','Peter', 'De Rijk',19.5);
		insert into "person" ("id","first_name","name","score")
			values ('jd','John', 'Do',17.5);
		insert into "person" ("id","first_name")
			values ('o','Oog');
		insert into "address" ("id","street", "number", "code", "city")
			values (1,'Universiteitsplein', '1', '2610', 'Wilrijk');
		insert into "address" ("id","street", "number", "code", "city")
			values (2,'Melkweg', '10', '1', 'Heelal');
		insert into "address" ("id","street", "number", "code")
			values (3,'Road', '0', '???');
		insert into "location" ("type", "inhabitant", "address")
			select 'work', "person"."id", "address"."id" from "person", "address"
			where "name" = 'De Rijk' and "street" = 'Universiteitsplein';
		insert into "location" ("type", "inhabitant", "address")
			select 'home', "person"."id", "address"."id" from "person", "address"
			where "name" = 'De Rijk' and "street" = 'Melkweg';
		insert into "location" ("type", "inhabitant", "address")
			select 'home', "person"."id", "address"."id" from "person", "address"
			where "name" = 'Do' and "street" = 'Road';
	}
	catch {$object serial set address id 4}
}

proc ::dbi::initdb {} {
	upvar object object
	upvar opt opt
	::dbi::cleandb
	::dbi::createdb
	::dbi::filldb
}

proc ::dbi::opendb {} {
	upvar object object
	upvar opt opt
	if {![::info exists opt(-openargs)]} {set opt(-openargs) {}}
	eval {$object open $opt(-testdb)} $opt(-openargs)
}

