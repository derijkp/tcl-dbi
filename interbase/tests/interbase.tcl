#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

set interface dbi
set version 0.1

package require dbi
package require dbi_interbase

set object [dbi_interbase]
set object2 [dbi_interbase]

interface test dbi_admin-0.1 $object \
	-testdb /home/ib/testdbi.gdb

array set opt [subst {
	-testdb /home/ib/testdbi.gdb
	-user2 PDR
	-object2 $object2
}]

eval interface test dbi-0.1 $object [array get opt]

::dbi::opendb
::dbi::initdb

interface::test {interface match} {
	$object supports
} {columnperm blobparams roles domains}

interface::test {transactions via exec} {
	$object exec {delete from "location";}
	$object exec {delete from "person";}
	set r1 [$object exec {select "first_name" from "person" order by "id";}]
	$object exec {set transaction}
	$object exec {
		insert into "person" values(1,'Peter','De Rijk',20);
	}
	$object exec {
		insert into "person" values(2,'John','Doe',17.5);
		insert into "person" ("id","first_name") values(3,'Jane');
	}
	set r2 [$object exec {select "first_name" from "person" order by "id";}]
	$object exec rollback
	set r3 [$object exec {select "first_name" from "person" order by "id";}]
	$object exec {set transaction}
	$object exec {
		insert into "person" values(1,'Peter','De Rijk',20);
	}
	$object exec {
		insert into "person" values(2,'John','Doe',17.5);
		insert into "person" ("id","first_name") values(3,'Jane');
	}
	$object exec commit
	set r4 [$object exec {select "first_name" from "person" order by "id";}]
	list $r1 $r2 $r3 $r4
} {{} {Peter John Jane} {} {Peter John Jane}}

interface::test {trigger with declare} {
	catch {$object exec {drop trigger person_Update}}
	catch {$object exec {delete from "person"}}
	$object exec {
		create trigger person_Update for "person" before update as
		declare variable current_project varchar(8);
		begin
			update "person"
			set "name" = NEW."name"
			where "id" = OLD."id";
		end
	}
	$object exec {update "person" set "first_name" = 'test' where "id" = 3}
} {}

$object destroy
$object2 destroy

interface testsummarize
