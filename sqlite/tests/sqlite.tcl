#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

set interface dbi
# $Format: "set version 0.$ProjectMajorVersion$"$
set version 0.8

package require dbi
package require dbi_sqlite

set object [dbi_sqlite]
set object2 [dbi_sqlite]

interface test dbi_admin-$version $object \
	-testdb test.db

interface::test {destroy without opening a database} {
	dbi_sqlite	db
	db destroy
	set a 1
} 1

array set opt [subst {
	-testdb test.db
	-user2 PDR
	-object2 $object2
}]

# $Format: "eval interface test dbi-0.$ProjectMajorVersion$ $object [array get opt]"$
eval interface test dbi-0.8 $object [array get opt]

::dbi::opendb
::dbi::initdb

interface::test {serial share} {
	$object exec {delete from "location";}
	$object exec {delete from "address";}
	$object exec {delete from "types"}
	catch {$object serial delete address id}
	catch {$object serial delete types i}
	$object serial add types i
	$object serial share address id types i
	$object exec {insert into "types" ("d") values (20)}
	$object exec {insert into "types" ("d") values (21)}
	$object exec {insert into "address" ("street") values ('test')}
	$object exec {select "id","street" from "address" order by "id"}
} {{3 test}}

$object destroy
$object2 destroy

interface testsummarize
