#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

set interface dbi
# $Format: "set version $ProjectMajorVersion$.$ProjectMinorVersion$"$
set version 1.0

package require interface
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

# $Format: "eval interface test dbi-$ProjectMajorVersion$.$ProjectMinorVersion$ $object [array get opt]"$
eval interface test dbi-1.0 $object [array get opt]

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

interface::test {get} {
	$object get {person pdr}
} {id pdr first_name Peter name {De Rijk} score 19.5}

interface::test {get error in object} {
	$object get person
} {object must be a list of the form {table id}} error

interface::test {get field} {
	$object get {person pdr} first_name
} {Peter}

interface::test {get field field} {
	$object get {person pdr} first_name name
} {Peter {De Rijk}}

interface::test {get field error} {
	$object get {person blabla} first_name
} {object {person blabla} not found} error

interface::test {set field} {
	$object set {person pdr} first_name Pe
	$object get {person pdr} first_name
} {Pe}

interface::test {set field} {
	$object set {person pdr} first_name Peter name test
	$object get {person pdr} first_name name
} {Peter test}

interface::test {set field list} {
	$object set {person pdr} {name {De Rijk}}
	$object get {person pdr} name
} {De Rijk}

interface::test {set field wrong # args error} {
	$object set {person pdr} test
} {data for set must be: field value ?field value ...?} error

interface::test {set non existing object} {
	$object set {person blabla} first_name Peter
	$object get {person blabla}
} {id blabla first_name Peter}

interface::test {insert} {
	catch {$object delete {person test}}
	$object insert person id test name test
	$object get {person test} name
} test

interface::test {insert list} {
	catch {$object delete {person test}}
	$object insert person {id test name test}
	$object get {person test} name
} test

interface::test {insert return} {
	catch {$object delete {person test}}
	$object insert person {id test}
} {person test}

interface::test {insert return serial} {
	$object insert address {street {test street}}
} {address [0-9]+} regexp

interface::test {insert with nullvalue} {
	catch {$object delete {person test}}
	$object insert -nullvalue NULL person id test name testname first_name NULL
	$object get {person test} first_name
} {}

interface::test {insert with first nullvalue} {
	catch {$object delete {person test}}
	$object insert -nullvalue NULL person first_name NULL id test name testname
	$object get {person test} first_name
} {}

interface::test {delete} {
	catch {$object insert person id test name test}
	$object delete {person test}
	$object get {person test} name
} {object {person test} not found} error

interface::test {unset} {
	catch {$object insert person id test name test}
	$object unset {person test} name
	$object get {person test} name
} {}

interface::test {set first field with nullvalue} {
	$object set -nullvalue NULL {person pdr} first_name NULL
	set result [$object get {person pdr} first_name]
	$object set {person pdr} first_name Peter
	set result
} {}

interface::test {set second field with nullvalue} {
	$object set -nullvalue NULL {person pdr} name {De Rijk} first_name NULL
	set result [$object get {person pdr} first_name]
	$object set {person pdr} name {De Rijk} first_name Peter
	set result
} {}

interface::test {set with special characters} {
	$object set {person blabla} first_name {pre'post;}
	$object get {person blabla} first_name
} {pre'post;}

interface::test {set (insert) with special characters} {
	$object exec {delete from "person" where "id" = 'blabla'}
	$object set {person blabla} first_name pre\'po\0st\;
	$object get {person blabla} first_name
} pre\'po\0st\;

$object destroy
$object2 destroy

interface testsummarize
