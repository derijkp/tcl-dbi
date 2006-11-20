#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

set interface dbi
# $Format: "set version $ProjectMajorVersion$.$ProjectMinorVersion$"$
set version 1.0

package require interface
package require dbi
package require dbi_sqlite3

set object [dbi_sqlite3]
set object2 [dbi_sqlite3]

array set opt [subst {
	-testdb test.db
	-user2 PDR
	-object2 $object2
}]

file delete $opt(-testdb)
source ../../tests/tools.tcl

dbi_sqlite3 db
db create $opt(-testdb)

::dbi::opendb
::dbi::createdb
::dbi::filldb

interface::test {transactions: syntax error in exec within transaction} {
	$object exec {delete from "location";}
	$object exec {delete from "person";}
	set r1 [$object exec {select "first_name" from "person";}]
	$object begin
	catch {$object exec {
		insert into "person" values(1,'Peter','De Rijk',19.5);
	}} error
	catch {$object exec -error {
		insert into "person" values(2,'John','Doe',18.5);
	}} error
	set r2 [$object exec {select "first_name" from "person";}]
	$object rollback
	set r3 [$object exec {select "first_name" from "person";}]
	list $r1 $r2 $r3 [string range $error 0 27]
} {{} Peter {} {bad option "-error": must be}} {skipon {![$object supports transactions]}}

interface::test {transactions: sql error in exec within transaction, seperate calls} {
	$object exec {delete from "location";}
	$object exec {delete from "person";}
	set r1 [$object exec {select "first_name" from "person";}]
	$object begin
	catch {$object exec {
		insert into "person" values(1,'Peter','De Rijk',19.5);
	}} error
	catch {$object exec {
		insert into "person" values(1,'John','Doe',error);
		insert into "person" ("id","first_name") values(3,'Jane');
	}} error
	set r2 [$object exec {select "first_name" from "person";}]
	$object rollback
	set r3 [$object exec {select "first_name" from "person";}]
	list $r1 $r2 $r3
} {{} Peter {}} {skipon {![$object supports transactions]}}
