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

interface test dbi_admin-$version $object \
	-testdb test.db

interface::test {destroy without opening a database} {
	dbi_sqlite3 db
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

interface::test {delete error} {
	$object delete {person test}
} {object {person test} not found} error

interface::test {delete trans} {
	$object commit
	$object exec {delete from person where name like 'test%'}
	set r1 [$object exec {select name from person where name like 'test%'}]
	$object begin
	catch {$object insert person id test name test}
	catch {$object insert person id test2 name test2}
	set r2 [$object exec {select name from person where name like 'test%'}]
	$object delete {person test}
	set r3 [$object exec {select name from person where name like 'test%'}]
	$object rollback
	set r4 [$object exec {select name from person where name like 'test%'}]
	list $r1 $r2 $r3 $r4
} {{} {test test2} test2 {}}

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

interface::test {unset} {
	catch {$object insert person id test name test}
	$object unset {person test} name
	$object get {person test} name
} {}

interface::test {unset 2 args} {
	$object set {person test} id test first_name first name test score 19.5
	$object unset {person test} name score
	$object get {person test}
} {id test first_name first}

interface::test {unset 3 args} {
	$object set {person test} id test first_name first name test score 19.5
	$object unset {person test} name score first_name
	$object get {person test}
} {id test}

interface::test {unset list} {
	$object set {person test} id test first_name first name test score 19.5
	$object unset {person test} {name score}
	$object get {person test}
} {id test first_name first}

interface::test {unset error constraint} {
	$object set {person test} id test first_name first name test score 19.5
	$object unset {person test} id
} {error unsetting fields on object identified by id = 'test' in table "person":
constraint failed} error

interface::test {unset error fields} {
	$object set {person test} id test first_name first name test score 19.5
	$object unset {person test} blabla
} {error preparing unset statement:
error no such column: blabla} error

interface::test {unset error} {
	catch {$object delete {person test}}
	$object unset {person test} name
} {object {person test} not found} error

interface::test {DICT collation} {
	catch {$object exec {delete from types}}
	foreach v {a20 a2 a1 a2b a4 a22 a10} {
		$object exec {insert into types(t) values(?)} $v
	}
	$object exec {select t from types order by t collate DICT}
} {a1 a2 a2b a4 a10 a20 a22}

interface::test {DICT collation vs DICTREAL} {
	catch {$object exec {delete from types}}
	foreach v {a20 a2.5 a1 a2b a4 a2.08 a10 a2.081 a2.0555} {
		$object exec {insert into types(t) values(?)} $v
	}
	$object exec {select t from types order by t collate DICT}
} {a1 a2.5 a2.08 a2.081 a2.0555 a2b a4 a10 a20}

interface::test {DICTREAL collation} {
	catch {$object exec {delete from types}}
	foreach v {a20 a2.5 a1 a2b a4 a2.08 a10 a2.081 a2.0555} {
		$object exec {insert into types(t) values(?)} $v
	}
	$object exec {select t from types order by t collate DICTREAL}
} {a1 a2.0555 a2.08 a2.081 a2.5 a2b a4 a10 a20}

interface::test {set and transactions} {
	catch {$object delete {person test}}
	catch {$object delete {person test2}}
	$object begin
	$object set {person test} first_name first name test score 19.5
	$object set {person test2} first_name first2
	$object rollback
	$object get {person test}
} {object {person test} not found} error

interface::test {function} {
	proc tfunc {a b} {return [list $a $b]}
	$object function tfunc tfunc
	$object exec -flat {select tfunc(1,2)}
} {{1 2}}

interface::test {collate} {
	$object exec {delete from "person" where "id" = 'blabla'}
	proc nocase_compare {a b} {
		return [string compare [string tolower $a] [string tolower $b]]
	}
	$object exec {insert into "person" values('jd2','john','do',18)}
	$object exec -flat {select "first_name" from "person" order by "first_name" collate nocase}
} {John john Oog Peter}

interface::test {regexp} {
	$object exec {select "first_name" from "person" where "first_name" regexp 'ohn'}
} {John john}

$object exec {insert into "person" ("id","first_name","name") values ('jd3','John',"Test Case")}
interface::test {list_concat} {
	$object exec {
		select "first_name",list_concat("name")
		from "person" where "first_name" regexp 'ohn' group by "first_name"
	}
} {{John {Do {Test Case}}} {john do}}

interface::test {altercolumn and dropcolumn} {
	set result {}
	$object exec {delete from "useof"}
	$object exec {insert into "useof"("id","person","score") values (1,"pdr",19)}
	$object exec {alter table "useof" add column tempcol text}
	array set a [$object info table useof]
	lappend result $a(type,tempcol)
	::dbi::sqlite3::altercolumn $object useof tempcol integer
	array set a [$object info table useof]
	lappend result $a(type,tempcol)
	::dbi::sqlite3::dropcolumn $object useof tempcol
	lappend result [$object fields useof]
	lappend result [$object exec {select * from "useof"}]
} {text integer {id person place usetime score score2} {{1 pdr {} {} 19.0 {}}}}

interface::test {backup and restore} {
	set tables [$object tables]
	set adata [list [$object exec {select * from address}]]
	catch {file delete $opt(-testdb).backup}
	$object backup $opt(-testdb).backup
	$object close
	catch {file delete $opt(-testdb)}
	$object create $opt(-testdb)
	$object open $opt(-testdb)
	lappend tables [$object tables]
	$object restore $opt(-testdb).backup
	lappend tables [$object tables]
	lappend adata [$object exec {select * from address}]
	list $tables $adata
} {{address location multi person types use v_test {} {address location multi person types use v_test}} {{{1 Universiteitsplein 1 2610 Wilrijk} {2 Melkweg 10 1 Heelal} {3 Road 0 ??? {}}} {{1 Universiteitsplein 1 2610 Wilrijk} {2 Melkweg 10 1 Heelal} {3 Road 0 ??? {}}}}}

interface::test {progress} {
	set ::progress 0
	proc testprogress {} {
		incr ::progress
		return 0
	}
	$object progress 4 testprogress
	$object exec {select * from address}
	expr {$::progress > 0}
} 1

interface::test {progress error} {
	proc testprogress {} {
		return 1
	}
	$object progress 4 testprogress
	$object exec {select * from address}
	expr {$::progress > 0}
} {1database error executing command "select * from address":
interrupted} error

$object destroy
$object2 destroy

interface testsummarize
