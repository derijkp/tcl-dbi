#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

package require dbi
package require dbi_interbase

set db [dbi_interbase]
set db2 [dbi_interbase]

interface test dbi/admin 0.1 $db \
	-testdb /home/ib/testdbi.gdb

interface test dbi/blob 0.1 $db \
	-testdb /home/ib/testdbi.gdb

interface test dbi 0.1 $db \
	-testdb /home/ib/testdbi.gdb -user2 PDR \
	-lines 0 \
	-object2 $db2

interface::opendb
interface::initdb

set interface::interface dbi_interbase
set interface::version 0.1

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

catch {$interface::object exec {create role "test"}}
catch {$interface::object exec {create role "try"}}

interface::test {info roles} {
	$object info roles
} {test try}

catch {$interface::object exec "grant \"test\" to [$interface::object info user]"}
catch {$interface::object exec "grant \"try\" to [$interface::object info user]"}

interface::test {info roles user} {
	$object info roles [$object info user]
} {test try}

$interface::object exec "revoke \"try\" from [$interface::object info user]"

interface::test {info roles user: one revoked} {
	$object info roles [$interface::object info user]
} {test}

catch {$interface::object exec "create domain \"tdom\" as varchar(6)"}
catch {$interface::object exec "create domain \"idom\" as integer"}

interface::test {info domains} {
	$object info domains
} {idom tdom}

interface::test {info domain} {
	$object info domain tdom
} {varchar(6) nullable}

$db destroy
$db2 destroy

interface::testsummarize
