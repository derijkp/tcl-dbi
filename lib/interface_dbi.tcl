package require interface

# $Format: "proc ::interfaces::dbi-0.$ProjectMajorVersion$ {option args} {"$
proc ::interfaces::dbi-0.8 {option args} {

interface::implement dbi $::dbi::version [file join $::dbi::dir doc xml interface_dbi.n.xml] {
	-testdb testdbi
	-user2 guest
	-object2 {}
	-openargs {}
} $option $args

# test interface command
# ----------------------

interface::test {interface match} {
	$object interface dbi
} $::dbi::version

interface::test {interface error} {
	$object interface test12134
} "$object does not support interface test12134" error

interface::test {interface list} {
	set list [$object interface]
	set pos [lsearch $list dbi]
	lrange $list $pos [expr {$pos+1}]
} [list dbi $::dbi::version]

# procedures used to setup databases
# ----------------------------------

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
	catch {$object exec {drop table "duse"}} result
	append fresult $result\n
	catch {$object exec {drop table "use"}} result
	append fresult $result\n
	catch {$object exec {drop table "test"}} result
	append fresult $result\n
	catch {$object exec {drop table "types"}} result
	append fresult $result\n
	catch {$object exec {drop table "location"}} result
	append fresult $result\n
	catch {$object exec {drop table "address"}} result
	append fresult $result\n
	catch {$object exec {drop table "person"}} result
	append fresult $result\n
	catch {$object exec {drop table bl}} result
	append fresult $result\n
	catch {$object exec {drop table "multi"}} result
	append fresult $result\n
	catch {$object exec {drop table "t"}} result
	append fresult $result\n
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
			check ("score2" > "score")}]
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
			values ('pdr','Peter', 'De Rijk',20.0);
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

# -------------------------------------------------------
# 							Tests
# -------------------------------------------------------
interface::test {open and close} {
	if {![::info exists opt(-openargs)]} {set opt(-openargs) {}}
	eval {$object open $opt(-testdb)} $opt(-openargs)
	$object close
} {}

interface::test {open error} {
	catch {$object close}
	catch {$object open xxxxx}
	$object exec {select * from test}
} {dbi object has no open database, open a connection first} error

interface::test {open -user error} {
	catch {$object close}
	catch {$object open $opt(-testdb) -user test -password afsg}
} 1

# Open test database for further tests
# ------------------------------------
::dbi::opendb

interface::test {create table} {
	::dbi::cleandb
	::dbi::createdb
	set ret {}
} {}

interface::test {create and fill table} {
	::dbi::cleandb
	::dbi::createdb
	::dbi::filldb
	set try {}
} {}

::dbi::initdb

interface::test {select} {
	$object exec {select * from "person"}
} {{pdr Peter {De Rijk} 20.0} {jd John Do 17.5} {o Oog {} {}}}

interface::test {select again} {
	$object exec {select * from "person"}
} {{pdr Peter {De Rijk} 20.0} {jd John Do 17.5} {o Oog {} {}}}

interface::test {select with -flat} {
	$object exec -flat {select * from "person"}
} {pdr Peter {De Rijk} 20.0 jd John Do 17.5 o Oog {} {}}

interface::test {select with -nullvalue} {
	$object exec -nullvalue NULL {select * from "person"}
} {{pdr Peter {De Rijk} 20.0} {jd John Do 17.5} {o Oog NULL NULL}}

interface::test {non select query with -nullvalue parameter} {
	catch {$object exec {delete from "person" where "id" = 'nul'}}
	$object exec -nullvalue {} {insert into "person" values (?,?,?,?)} nul hasnull {} {}
	set result [$object exec -nullvalue NULL {select * from "person" where "id" = 'nul'}]
	$object exec {delete from "person" where "id" = 'nul'}
	set result	
} {{nul hasnull NULL NULL}}

interface::test {non select query without -nullvalue parameter} {
	catch {$object exec {delete from "person" where "id" = 'nul'}}
	$object exec {insert into "person" values (?,?,?,?)} nul hasnull {} 10.0
	set result [$object exec -nullvalue NULL {select * from "person" where "id" = 'nul'}]
	$object exec {delete from "person" where "id" = 'nul'}
	set result	
} {{nul hasnull {} 10.0}}

interface::test {non select query with -nullvalue update to NULL} {
	catch {$object exec {delete from "person" where "id" = 'nul'}}
	$object exec -nullvalue {} {insert into "person" values (?,?,?,?)} nul hasnull {} 10.0
	lappend result [lindex [$object exec -nullvalue NULL {select * from "person" where "id" = 'nul'}] 0]
	$object exec -nullvalue {} {update "person" set "score" = ? where "id" = 'nul'} {}
	lappend result [lindex [$object exec -nullvalue NULL {select * from "person" where "id" = 'nul'}] 0]
	$object exec {delete from "person" where "id" = 'nul'}
	set result	
} {{nul hasnull NULL 10.0} {nul hasnull NULL NULL}}

interface::test {error: select "try" from "person"} {
	$object exec {select "try" from "person"}
} {* while executing command: "select "try" from "person""} error match

interface::test {select fetch} {
	$object exec -usefetch {select * from "person"}
} {}

interface::test {1 fetch} {
	$object exec -usefetch {select * from "person"}
	$object fetch
} {pdr Peter {De Rijk} 20.0}

interface::test {2 fetch} {
	$object exec -usefetch {select * from "person"}
	$object fetch
	$object fetch
} {jd John Do 17.5}

interface::test {3 fetch} {
	$object exec -usefetch {select * from "person"}
	$object fetch
	$object fetch
	$object fetch
} {o Oog {} {}}

interface::test {4 fetch, end} {
	$object exec -usefetch {select * from "person"}
	$object fetch
	$object fetch
	$object fetch
	$object fetch
} {line 3 out of range} error

interface::test {5 fetch, end} {
	$object exec -usefetch {select * from "person"}
	$object fetch
	$object fetch
	$object fetch
	catch {$object fetch}
	$object fetch
} {line 3 out of range} error

interface::test {fetch and two objects} {
	if {[string equal $opt(-object2) ""]} {
		error "no -object2 option given"
	}
	eval {$opt(-object2) open $opt(-testdb)} $opt(-openargs)
	$object exec -usefetch {select * from "person"}
	$opt(-object2) info table person
	$opt(-object2) close
	$object fetch
} {pdr Peter {De Rijk} 20.0}

# testing for fetching by number: is not always supported
# -------------------------------------------------------

interface::test {fetch 1} {
	$object exec -usefetch {select * from "person"}
	$object fetch 1
} {jd John Do 17.5}

interface::test {repeat fetch 1} {
	$object exec -usefetch {select * from "person"}
	$object fetch 1
	$object fetch 1
} {jd John Do 17.5}

interface::test {fetch 1 1} {
	$object exec -usefetch {select * from "person"}
	$object fetch 1 1
} {John}

interface::test {fetch with NULL} {
	$object exec -usefetch {select * from "person"}
	$object fetch 2
} {o Oog {} {}}

interface::test {fetch with -nullvalue} {
	$object exec -usefetch {select * from "person"}
	$object fetch -nullvalue NULL 2
} {o Oog NULL NULL}

interface::test {fetch lines} {
	$object exec -usefetch {select * from "person"}
	$object fetch lines
} {3} {skipon {![$object supports lines]}}

interface::test {fetch isnull 1} {
	$object exec -usefetch {select * from "person"}
	$object fetch isnull 2 2
} 1

interface::test {fetch isnull 1} {
	$object exec -usefetch {select * from "person"}
	$object fetch isnull current 2
} {line -1 out of range} error

interface::test {fetch isnull 1} {
	$object exec -usefetch {select * from "person"}
	$object fetch
	$object fetch
	$object fetch
	$object fetch isnull current 2
} 1

interface::test {fetch isnull 0} {
	$object exec -usefetch {select * from "person"}
	$object fetch isnull 2 1
} 0

interface::test {fetch field out of range} {
	$object exec -usefetch {select * from "person"}
	$object fetch 2 4
} {field 4 out of range} error

interface::test {fetch line out of range} {
	$object exec -usefetch {select * from "person"}
	$object fetch 3
} {line 3 out of range} error

interface::test {fetch isnull field out of range} {
	$object exec -usefetch {select * from "person"}
	$object fetch isnull 2 5
} {field 5 out of range} error

interface::test {fetch pos} {
	$object exec -usefetch {select * from "person"}
	$object fetch pos
} -1

interface::test {fetch pos 2} {
	$object exec -usefetch {select * from "person"}
	$object fetch 0
	$object fetch pos
} 0

interface::test {fetch pos 3} {
	$object exec -usefetch {select * from "person"}
	$object fetch
	$object fetch pos
} 0

interface::test {fetch current after fetch 0} {
	$object exec -usefetch {select * from "person"}
	$object fetch 0
	$object fetch current
	$object fetch current
} {pdr Peter {De Rijk} 20.0}

interface::test {fetch current after fetch} {
	$object exec -usefetch {select * from "person"}
	$object fetch
	$object fetch current
	$object fetch current
} {pdr Peter {De Rijk} 20.0}

interface::test {fetch fields} {
	$object exec -usefetch {select * from "person"}
	$object fetch fields
} {id first_name name score}

interface::test {fetch fields limited select} {
	$object exec -usefetch {select "id","score" from "person"}
	$object fetch fields
} {id score}

interface::test {fetch fields when no result} {
	$object exec -usefetch {select * from "person" where "id" = 'blabla'}
	$object fetch fields
} {id first_name name score}

interface::test {fetch with no fetch result available} {
	$object exec {select * from "person"}
	$object fetch
} {no result available: invoke exec method with -usefetch option first} error

interface::test {deal with error in fetch lines} {
	$object exec -usefetch {select * from "person"}
	catch {$object fetch lines}
	$object fetch
	$object fetch
} {jd John Do 17.5}

interface::test {backfetch} {
	$object exec -usefetch {select * from "person"}
	catch {$object fetch lines}
	$object fetch
	$object fetch
	$object fetch 0
} {pdr Peter {De Rijk} 20.0} {skipon {![$object supports backfetch]}}

interface::test {fetch and begin/rollback} {
	$object exec -usefetch {select * from "person"}
	$object fetch
	$object begin
	$object exec {
		insert into "person"("id","first_name","name") values(10,'Try','It');
	}
	$object fetch
} {no result available: invoke exec method with -usefetch option first} error

$object rollback

interface::test {fetch after select error} {
	catch {$object exec -usefetch {select * from "Idonotexist"}}
	$object fetch
} {no result available: invoke exec method with -usefetch option first} error

# parameters
# ----------

interface::test {parameter} {
	$object exec {select * from "person" where "name" = ?} {De Rijk}
} {{pdr Peter {De Rijk} 20.0}}

interface::test {parameters} {
	$object exec {select * from "person" where "name" = ? and "score" = ?} {De Rijk} 20
} {{pdr Peter {De Rijk} 20.0}}

interface::test {char parameter} {
	$object exec {select * from "person" where "id" = ?} pdr
} {{pdr Peter {De Rijk} 20.0}}

interface::test {parameters} {
	$object exec {select * from "person" where "name" = ? and "score" = ?} {De Rijk} 19
} {}

interface::test {parameters error} {
	$object exec {select * from "person" where "name" = ?}
} {wrong number of arguments given to exec while executing command: "select * from "person" where "name" = ?"} error

interface::test {parameters with comments and literals} {
	$object exec {select "id",'?' /* selecting what ? */ from "person" where "name" = ? and "score" = ?} {De Rijk} 20
} {{pdr ?}}

interface::test {parameters with 2 quotes} {
	$object exec {delete from "person"  where "id" = ?} test
	$object exec {insert into "person" ("id","name") values(?,?)} test {a'b'}
	$object exec -flat {select "name" from "person" where "id" = ?} test
} {a'b'}

interface::test {parameters with 6 quotes} {
	$object exec {delete from "person"  where "id" = ?} test
	$object exec {insert into "person" ("id","name") values(?,?)} test {a'b'c'd'e'f'}
	$object exec -flat {select "name" from "person" where "id" = ?} test
} {a'b'c'd'e'f'}

# serial
# ------

dbi::initdb

interface::test {serial basic} {
	$object exec {delete from "location";}
	$object exec {delete from "types"}
	catch {$object serial delete types i}
	$object serial add types i
	$object exec {insert into "types" ("d") values (20.0)}
	$object exec {insert into "types" ("d") values (21.0)}
	$object exec {select "i","d" from "types" order by "d"}
} {{1 20.0} {2 21.0}} {skipon {![$object supports serials]}}

interface::test {serial set} {
	$object exec {delete from "types"}
	catch {$object serial delete types i}
	$object serial add types i 1
	$object exec {insert into "types" ("d") values (20.0)}
	$object serial set "types" i 8
	set i [$object serial set types i]
	$object exec {insert into "types" ("d") values (21.0)}
	list $i [$object exec {select "i","d" from "types" order by "d"}]
} {8 {{2 20.0} {9 21.0}}} {skipon {![$object supports serials]}}

interface::test {serial overrule} {
	$object exec {delete from "types"}
	catch {$object serial delete types i}
	$object serial add types i 1
	$object exec {insert into "types" ("d") values (20.0)}
	$object exec {insert into "types" ("i","d") values (9,21.0)}
	$object exec {select "i","d" from "types" order by "d"}
} {{2 20.0} {9 21.0}} {skipon {![$object supports serials]}}

interface::test {serial next} {
	$object exec {delete from "types"}
	catch {$object serial delete types i}
	$object serial add types i
	$object exec {insert into "types" ("d") values (20.0)}
	set i [$object serial next types i]
	$object exec {insert into "types" ("d") values (20.1)}
	list $i [$object exec {select "i" from "types" order by "d"}]
} {2 {1 3}} {skipon {![$object supports serials]}}

interface::test {serial cache error test} {
	$object exec {delete from "types"}
	catch {$object serial delete types i}
	$object serial add types i
	$object exec {insert into "types"("vc") values('a')}
	set pos1 [$object serial set types i]
	$object exec {delete from "types"}
	catch {$object serial delete types i}
	$object serial add types i
	$object exec {insert into "types"("vc") values('a')}
	set pos2 [$object serial set types i]
	expr {$pos1 == $pos2}
} 1 {skipon {![$object supports serials]}}

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
} {{3 test}} {skipon {![$object supports sharedserials]}}

dbi::initdb

# storing and retrieving different types
# --------------------------------------

interface::test {types} {
	set error ""
	foreach {field value} [list i 20 si 10 vc test c test f 18.5 d 19.5 da "2000-11-18" t 10:40:30.000 ts "2000-11-18 10:40:30.000"] {
		# puts [list $field $value]
		$object exec {delete from "types"}
		$object exec "insert into \"types\"(\"$field\") values(?)" $value
		set rvalue [lindex [lindex [$object exec "select \"$field\" from \"types\" where \"$field\" = ?" $value] 0] 0]
		if {"$value" != "$rvalue"} {
			append error "different values \"$value\" and \"$rvalue\" for $field\n"
		}
		# puts [list $field $rvalue]
	}
	if [string length $error] {error $error}
	set a 1
} 1

# information about the db
# ------------------------

dbi::initdb

interface::test {tables 2} {
	# there maybe other tables than these present
	array set a {address 1 location 1 person 1 types 1 use 1 v_test 1}
	set tables {}
	foreach table [$object tables] {
		if {[info exists a($table)]} {lappend tables $table}
	}
	lsort $tables
} {address location person types use*} match

interface::test {special table} {
	catch {$object exec {create table "o$test" (i integer)}}
	set result [lsort [$object tables]]
	$object exec {drop table "o$test"}
	set result
} {address location multi {o$test} person types use*} match

interface::test {db fields} {
	$object fields person
} {id first_name name score}

interface::test {info error db fields} {
	$object fields notexist
} {table "notexist" does not exist} error

interface::test {table info} {
	array set a [$object info table use]
	list $a(fields) $a(type,id) $a(length,place) [array names a primary,*]
} {{id person place usetime score score2} integer 100 primary,id}

interface::test {table info 2} {
	array set a [$object info table types]
	set result ""
	foreach name [lsort [array names a type,*]] {
		lappend result $a($name)
	}
	set result
} {char double date float integer smallint time timestamp varchar}

interface::test {info views} {
	$object info views
} {v_test} {skipon {![$object supports views]}}

interface::test {info access select} {
	$object exec "grant select on \"person\" to \"$opt(-user2)\""
	list [$object info access select $opt(-user2)] [lsort [$object info access select [$object info user]]]
} {person {address location person types use v_test}} {skipon {![$object supports permissions]}} {skipon {![$object supports lines]}}

interface::test {info access select table} {
	$object exec "grant select on \"person\" to \"$opt(-user2)\""
	list [$object info access select $opt(-user2) person] [$object info access select [$object info user] person]
} {{id first_name name score} {id first_name name score}} {skipon {![$object supports permissions]}}

interface::test {info access insert} {
	$object exec "grant insert on \"person\" to \"$opt(-user2)\""
	list [$object info access insert $opt(-user2)] [$object info access insert [$object info user]]
} {person {address location multi person types use v_test}} {skipon {![$object supports permissions]}}

interface::test {info access update} {
	$object exec "revoke update on \"person\" from $opt(-user2)"
	$object exec "grant update on \"person\" to $opt(-user2)"
	list [$object info access update $opt(-user2)] [$object info access update [$object info user]]
} {person {address location multi person types use v_test}} {skipon {![$object supports permissions]}}

interface::test {info access select table} {
	list [$object info access select $opt(-user2) use] [$object info access select $opt(-user2) person] [$object info access select [$object info user] person]
} {{} {id first_name name score} {id first_name name score}} {skipon {![$object supports permissions]}}

interface::test {info access update table} {
	$object exec "revoke update on \"person\" from $opt(-user2)"
	$object exec "grant update(\"id\",\"first_name\") on \"person\" to $opt(-user2)"
	list [lsort [$object info access update $opt(-user2) person]] [lsort [$object info access update [$object info user] person]]
} {{first_name id} {first_name id name score}} {skipon {![$object supports columnperm]}} {skipon {![$object supports permissions]}}

interface::test {info referenced} {
	$object info referenced person
} {location {inhabitant id} use {person id}} {skipon {![$object supports columnperm]}} {skipon {![$object supports foreignkeys]}}

interface::test {info views should not mess up a resultset} {
	$object exec -usefetch {select * from "person"}
	$object fetch
	set views [$object info views]
	list $views [$object fetch]
} {v_test {jd John Do 17.5}} {skipon {![$object supports views]}}

interface::test {info fields should not mess up a resultset} {
	$object exec -usefetch {select * from "person"}
	$object fetch
	set fields [$object info fields person]
	list $fields [$object fetch]
} {{id first_name name score} {jd John Do 17.5}}

interface::test {error when info on closed object} {
	$object close
	set fields [$object info fields person]
} {dbi object has no open database, open a connection first} error
catch {::dbi::opendb}

# caching
# -------

interface::test {drop table not after -cache} {
	$object exec {create table "t" (i integer)}
	$object exec {insert into "t" values(1)}
	$object exec {select * from "t"}
	$object exec {drop table "t"}
	lsearch [$object tables] t
} -1

interface::test {drop table after -cache} {
	$object exec {create table "t" (i integer)}
	$object exec {insert into "t" values(1)}
	$object exec -cache {select * from "t"}
	$object exec {drop table "t"}
	lsearch [$object tables] t
} -1

interface::test {-cache simple} {
	puts 1:[time {$object exec {select * from "person" where "name" = 'De Rijk'}}]
	puts 2:[time {$object exec {select * from "person" where "name" = 'De Rijk'}}]
	puts cache1:[time {$object exec -cache {select * from "person" where "name" = 'De Rijk'}}]
	puts cache2:[time {$object exec -cache {select * from "person" where "name" = 'De Rijk'}}]
	$object exec -cache {select * from "person" where "name" = 'De Rijk'}
} {{pdr Peter {De Rijk} 20.0}}

interface::test {-cache parameter} {
	puts 1:[time {$object exec {select * from "person" where "name" = ?} {De Rijk}}]
	puts 2:[time {$object exec {select * from "person" where "name" = ?} {De Rijk}}]
	puts cache1:[time {$object exec -cache {select * from "person" where "name" = ?} {De Rijk}}]
	puts cache2:[time {$object exec -cache {select * from "person" where "name" = ?} {De Rijk}}]
	$object exec -cache {select * from "person" where "name" = ?} {De Rijk}
} {{pdr Peter {De Rijk} 20.0}}

interface::test {-cache parameter mix} {
	$object exec -cache {select * from "person" where "name" = ?} {De Rijk}
	$object exec -cache {select * from "address" where "number" = ?} 0
	$object exec -cache {select * from "person" where "name" = ?} Do
	$object exec -cache {select * from "address" where "number" = ?} 1
} {{1 Universiteitsplein 1 2610 Wilrijk}}

interface::test {-cache parameter mix 2} {
	$object exec -cache {select * from "person" where "name" = ?} {De Rijk}
	$object exec -cache {select * from "address" where "number" = ?} 0
	$object exec -cache {select * from "person" where "name" = ?} Do
	$object exec -cache {select * from "address" where "number" = ?} 1
	$object exec -cache {select * from "person" where "name" = ?} {De Rijk}
} {{pdr Peter {De Rijk} 20.0}}

interface::test {-cache parameter mix with plain} {
	$object exec -cache {select * from "person" where "name" = ?} {De Rijk}
	$object exec -cache {select * from "address" where "number" = ?} 0
	$object exec -cache {select * from "person" where "name" = ?} Do
	$object exec {select * from "person" where "name" = ?} {De Rijk}
	$object exec {select * from "address" where "number" = ?} 1
	$object exec -cache {select * from "address" where "number" = ?} 1
	$object exec -cache {select * from "person" where "name" = ?} {De Rijk}
} {{pdr Peter {De Rijk} 20.0}}

interface::test {-cache parameter mix with plain 2} {
	$object exec {select * from "person" where "name" = ?} {De Rijk}
	$object exec {select * from "person" where "name" = ?} {De Rijk}
	$object exec -cache {select * from "person" where "name" = ?} {De Rijk}
	$object exec -cache {select * from "person" where "name" = ?} {De Rijk}
	$object exec -cache {select * from "person" where "name" = ?} {De Rijk}
} {{pdr Peter {De Rijk} 20.0}}

# clones
# ------
 ::dbi::initdb

interface::test {close parent distroys clones} {
	set clone1 [$object clone]
	set clone2 [$object clone]
	$object close
	$clone2 tables
} {^invalid command name} error regexp

catch {::dbi::opendb}

interface::test {clones} {
	foreach clone [$object clones] {$clone close}
	lappend clist [$object clone]
	lappend clist [$object clone]
	string equal $clist [$object clones]
} 1

interface::test {delete clones} {
	foreach clone [$object clones] {$clone close}
	catch {$clone tables} error
	set test [regexp {^invalid command name} $error]
	list $test [$object clones]
} {1 {}}

interface::test {clone may not use open} {
	foreach clone [$object clones] {$clone close}
	set clone [$object clone]
	$clone open $opt(-testdb)
} {clone may not use open} error

interface::test {close first clone} {
	foreach clone [$object clones] {$clone close}
	set clone1 [$object clone]
	set clone2 [$object clone]
	$clone1 close
	string equal [$object clones] [list $clone2]
} 1

interface::test {close last clone} {
	foreach clone [$object clones] {$clone close}
	set clone1 [$object clone]
	set clone2 [$object clone]
	$clone2 close
	string equal [$object clones] [list $clone1]
} 1

interface::test {clone and object give same tables information} {
	foreach clone [$object clones] {$clone close}
	set clone [$object clone]
	set clist [$clone tables]
	set olist [$object tables]
	$clone close
	string equal $clist $olist
} 1

interface::test {-cache parameter in clone} {
	set clone [$object clone]
	$clone exec {select * from "person" where "name" = ?} {De Rijk}
	$clone exec {select * from "person" where "name" = ?} {De Rijk}
	$clone exec -cache {select * from "person" where "name" = ?} {De Rijk}
	$clone exec -cache {select * from "person" where "name" = ?} {De Rijk}
	set result [$clone exec -cache {select * from "person" where "name" = ?} {De Rijk}]
	$clone close
	set result
} {{pdr Peter {De Rijk} 20.0}}

interface::test {-cache parameter in clone, mix} {
	set clone [$object clone]
	$clone exec {select * from "person" where "name" = ?} {De Rijk}
	$clone exec {select * from "person" where "name" = ?} {De Rijk}
	$clone exec -cache {select * from "person" where "name" = ?} {De Rijk}
	$object exec {select * from "person" where "name" = ?} {De Rijk}
	set result [$clone exec -cache {select * from "person" where "name" = ?} {De Rijk}]
	$clone close
	set result
} {{pdr Peter {De Rijk} 20.0}}

::dbi::initdb
interface::test {clone with some fetching} {
	foreach clone [$object clones] {$clone close}
	set clone [$object clone]
	set olist [$object tables]
	$object exec -usefetch {select * from "person"}
	set l1 [$object fetch]
	set clist [$clone tables]
	set l2 [$object fetch]
	$clone close
	list $l1 $l2 [string equal $clist $olist]
} {{pdr Peter {De Rijk} 20.0} {jd John Do 17.5} 1}

interface::test {clone of clone goes to parent} {
	foreach clone [$object clones] {$clone close}
	set clone1 [$object clone]
	set clone2 [$clone1 clone]
	set clones [$object clones]
	set num [llength $clones]
	set test1 [string equal [lsort [$object clones]] [lsort [list $clone1 $clone2]]]
	set test2 [string equal [$clone1 parent] $object]
	set test3 [string equal [$clone2 parent] $object]
	list $num $test1 $test2 $test3
} {2 1 1 1}

interface::test {parent} {
	string equal $object [$object parent]
} 1

interface::test {clones error} {
	set clone [$object clone]
	$clone clones
} {error: object "*" is a clone} error match

interface::test {clone and object must be able to mix fetches} {
	set clone [$object clone]
	set result {}
	$object exec -usefetch {select * from "person"}
	$clone exec -usefetch {select * from "location"}
	lappend result [$object fetch]
	lappend result [$clone fetch]
	lappend result [$object fetch]
	lappend result [$clone fetch]
	$clone close
	set result
} {{pdr Peter {De Rijk} 20.0} {work pdr 1} {jd John Do 17.5} {home pdr 2}}

interface::test {clone and object must be able to mix fetches within a transaction} {
	set clone [$object clone]
	set result {}
	$object begin
	$object exec -usefetch {select * from "person"}
	$clone exec -usefetch {select * from "location"}
	lappend result [$object fetch]
	lappend result [$clone fetch]
	lappend result [$object fetch]
	lappend result [$clone fetch]
	$object rollback
	$clone close
	set result
} {{pdr Peter {De Rijk} 20.0} {work pdr 1} {jd John Do 17.5} {home pdr 2}}

interface::test {info may be implemented using a clone, see if it is respawned ok} {
	set fields [$object info fields person]
	$object close
	::dbi::opendb
	$object info fields person
} {id first_name name score}

interface::test {info may be implemented using a clone, see if it is respawned ok, views} {
	set views [$object info views]
	$object close
	::dbi::opendb
	$object info views
} v_test {skipon {![$object supports views]}}

interface::test {clone transaction sharing} {
	set clone [$object clone]
	$object begin
	$clone exec {
		insert into "person" ("id","first_name","name","score")
			values ('new','new','test',20.0);
	}
	set oresult [$object exec {select * from "person" where "id" = 'new'}]
	set cresult [$clone exec {select * from "person" where "id" = 'new'}]
	$object rollback
	set orresult [$clone exec {select * from "person" where "id" = 'new'}]
	set crresult [$clone exec {select * from "person" where "id" = 'new'}]
	list $oresult $cresult $orresult $crresult
} {{{new new test 20.0}} {{new new test 20.0}} {} {}} {skipon {![$object supports sharedtransactions]}}

interface::test {clone transaction sharing from clone} {
	set clone [$object clone]
	$object begin
	$object exec {
		insert into "person" ("id","first_name","name","score")
			values ('new','new','test',20.0);
	}
	set oresult [$object exec {select * from "person" where "id" = 'new'}]
	set cresult [$clone exec {select * from "person" where "id" = 'new'}]
	$clone rollback
	set orresult [$object exec {select * from "person" where "id" = 'new'}]
	set crresult [$clone exec {select * from "person" where "id" = 'new'}]
	list $oresult $cresult $orresult $crresult
} {{{new new test 20.0}} {{new new test 20.0}} {} {}} {skipon {![$object supports sharedtransactions]}}

interface::test {clone transaction sharing test without clone} {
	$object begin
	$object exec {
		insert into "person" ("id","first_name","name","score")
			values ('new','new','test',20.0);
	}
	set oresult [$object exec {select * from "person" where "id" = 'new'}]
	$object rollback
	set orresult [$object exec {select * from "person" where "id" = 'new'}]
	list $oresult $orresult
} {{{new new test 20.0}} {}} {skipon {![$object supports transactions]}}

# transactions
# ------------

$object rollback

interface::test {transactions} {
	$object exec {delete from "location";}
	$object exec {delete from "person";}
	set r1 [$object exec {select "first_name" from "person" order by "id";}]
	$object begin
	$object exec {
		insert into "person" values(1,'Peter','De Rijk',20.0);
	}
	$object exec {
		insert into "person" values(2,'John','Doe',17.5);
		insert into "person" ("id","first_name") values(3,'Jane');
	}
	set r2 [$object exec {select "first_name" from "person" order by "id";}]
	$object rollback
	set r3 [$object exec {select "first_name" from "person" order by "id";}]
	$object begin
	$object exec {
		insert into "person" values(1,'Peter','De Rijk',20.0);
	}
	$object exec {
		insert into "person" values(2,'John','Doe',17.5);
		insert into "person" ("id","first_name") values(3,'Jane');
	}
	$object commit
	set r4 [$object exec {select "first_name" from "person" order by "id";}]
	list $r1 $r2 $r3 $r4
} {{} {Peter John Jane} {} {Peter John Jane}} {skipon {![$object supports transactions]}}

interface::test {autocommit error} {
	$object exec {delete from "location";}
	$object exec {delete from "person";}
	set r1 [$object exec {select "first_name" from "person";}]
	catch {$object exec {
		insert into "person" values(1,'Peter','De Rijk',20.0);
		insert into "person" values(1,'John','Doe','error');
		insert into "person" ("id","first_name") values(3,'Jane');
	}}
	list $r1 [$object exec {select "first_name" from "person";}]
} {{} {}} {skipon {![$object supports transactions]}}

interface::test {transactions: syntax error in exec within transaction} {
	$object exec {delete from "location";}
	$object exec {delete from "person";}
	set r1 [$object exec {select "first_name" from "person";}]
	$object begin
	catch {$object exec {
		insert into "person" values(1,'Peter','De Rijk',20.0);
	}} error
	catch {$object exec -error {
		insert into "person" values(2,'John','Doe',18.5);
	}} error
	set r2 [$object exec {select "first_name" from "person";}]
	$object rollback
	set r3 [$object exec {select "first_name" from "person";}]
	list $r1 $r2 $r3 [string range $error 0 27]
} {{} Peter {} {bad option "-error": must be}} {skipon {![$object supports transactions]}}

interface::test {transactions: sql error in exec within transaction} {
	$object exec {delete from "location";}
	$object exec {delete from "person";}
	set r1 [$object exec {select "first_name" from "person";}]
	$object begin
	catch {$object exec {
		insert into "person" values(1,'Peter','De Rijk',20.0);
		insert into "person" values(1,'John','Doe','error');
		insert into "person" ("id","first_name") values(3,'Jane');
	}} error
	set r2 [$object exec {select "first_name" from "person";}]
	$object rollback
	set r3 [$object exec {select "first_name" from "person";}]
	list $r1 $r2 $r3
} {{} Peter {}} {skipon {![$object supports transactions]}}

interface::test {transactions: sql error in exec within transaction, seperate calls} {
	$object exec {delete from "location";}
	$object exec {delete from "person";}
	set r1 [$object exec {select "first_name" from "person";}]
	$object begin
	catch {$object exec {
		insert into "person" values(1,'Peter','De Rijk',20.0);
	}} error
	catch {$object exec {
		insert into "person" values(1,'John','Doe','error');
		insert into "person" ("id","first_name") values(3,'Jane');
	}} error
	set r2 [$object exec {select "first_name" from "person";}]
	$object rollback
	set r3 [$object exec {select "first_name" from "person";}]
	list $r1 $r2 $r3
} {{} Peter {}} {skipon {![$object supports transactions]}}

# roles
# ------------

if {[$object supports roles]} {
	catch {$object exec {create role "test"}}
	catch {$object exec {create role "try"}}
}

interface::test {info roles} {
	$object info roles
} {test try} {skipon {![$object supports roles]}}

if {[$object supports roles]} {
	catch {$object exec "grant \"test\" to [$object info user]"}
	catch {$object exec "grant \"try\" to [$object info user]"}
}

interface::test {info roles user} {
	$object info roles [$object info user]
} {test try} {skipon {![$object supports roles]}}

if {[$object supports roles]} {
	$object exec "revoke \"try\" from [$object info user]"
}

interface::test {info roles user: one revoked} {
	$object info roles [$object info user]
} {test} {skipon {![$object supports roles]}}

# domains
# -------

if {[$object supports domains]} {
	catch {$object exec "create domain \"tdom\" as varchar(6)"}
	catch {$object exec "create domain \"idom\" as integer"}
}

interface::test {info domains} {
	$object info domains
} {idom tdom} {skipon {![$object supports domains]}}

interface::test {info domain} {
	$object info domain tdom
} {varchar(6) nullable} {skipon {![$object supports domains]}}

$object close

}

