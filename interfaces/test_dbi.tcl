
# dbi 0.1 interface testing
# -------------------------

set ::interface::name $interface-$version
::interface::options $::interface::name {
	testdb -testdb testdbi
	user2 -user2 guest
	lines -lines 1
	columnperm -columnperm 1
	object2 -object2 {}
}

# test interface command
# ----------------------

interface::test {interface match} {
	$object interface dbi
} {0.1}

interface::test {interface error} {
	$object interface test12134
} "$::interface::object does not support interface test12134" 1

interface::test {interface list} {
	set list [$object interface]
	set pos [lsearch $list dbi]
	lrange $list $pos [expr {$pos+1}]
} {dbi 0.1}

# procedures used to setup databases
# ----------------------------------

proc ::interface::cleandb {} {
	variable object
	$object rollback
	set fresult ""
	catch {$object serial delete test id} result
	append fresult $result\n
	catch {$object serial delete address id} result
	append fresult $result\n
	catch {$object exec {drop view "v_test"}} result
	append fresult $result\n
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
	return $fresult
}

proc ::interface::createdb {} {
	variable object
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
		create table "location" (
			"type" varchar(100) check ("type" in ('home','work','leisure')),
			"inhabitant" char(6) references "person"("id"),
			"address" integer references "address"("id")
		);
	}
	$object exec {
		create table "use" (
			"id" integer not null primary key,
			"person" integer not null unique references "person"("id"),
			"place" varchar(100),
			"usetime" timestamp,
			"score" float check ("score" < 20.0),
			"score2" float check ("score" < 20.0),
			check ("score2" > "score")
		);
	}
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
	$object exec {create view "v_test" as select "id", "first_name", "name" from "person"}
	catch {$object serial add address id}
}

proc ::interface::filldb {} {
	variable object
	$object exec {
		insert into "person" ("id","first_name","name","score")
			values ('pdr','Peter', 'De Rijk',20);
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

proc ::interface::initdb {} {
	cleandb
	createdb
	filldb
}

proc ::interface::opendb {} {
	variable object
	variable testdb
	$object open $testdb
}

# -------------------------------------------------------
# 							Tests
# -------------------------------------------------------
interface::test {open and close} {
	variable testdb
	$object open $testdb
	$object close
} {}

interface::test {open error} {
	catch {$object close}
	catch {$object open xxxxx}
	$object exec {select * from test}
} {dbi object has no open database, open a connection first} 1

interface::test {open -user error} {
	variable testdb
	catch {$object close}
	catch {$object open $testdb -user test -password afsg}
} 1

# Open test database for further tests
# ------------------------------------
interface::opendb

interface::test {create table} {
	cleandb
	createdb
	set ret {}
} {}

interface::test {create and fill table} {
	cleandb
	createdb
	filldb
	set try {}
} {}

interface::initdb

interface::test {select} {
	$object exec {select * from "person"}
} {{pdr Peter {De Rijk} 20.0} {jd John Do 17.5} {o Oog {} {}}}

interface::test {select again} {
	$object exec {select * from "person"}
} {{pdr Peter {De Rijk} 20.0} {jd John Do 17.5} {o Oog {} {}}}

interface::test {select with -nullvalue} {
	$object exec -nullvalue NULL {select * from "person"}
} {{pdr Peter {De Rijk} 20.0} {jd John Do 17.5} {o Oog NULL NULL}}

interface::test {error} {
	if ![catch {$object exec {select "try" from "person"}} result] {
		error "test should cause an error"
	}
	if ![regexp {while executing command: "select "try" from "person""} $result] {
		set error "The error given does not contain:\n"
		append error {while executing command: "select "try" from "person""}
		append error "error was:\n$result"
		error $error
	}
	set a 1
} 1

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
} {line 3 out of range} 1

interface::test {5 fetch, end} {
	$object exec -usefetch {select * from "person"}
	$object fetch
	$object fetch
	$object fetch
	catch {$object fetch}
	$object fetch
} {line 3 out of range} 1

interface::test {fetch and two objects} {
	variable object2
	if {[string equal $object2 ""]} {
		error "no -object2 option given"
	}
	$object2 open $::interface::testdb
	$object exec -usefetch {select * from "person"}
	$object2 info table person
	$object2 close
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

if $::interface::lines {
	interface::test {fetch lines} {
		$object exec -usefetch {select * from "person"}
		$object fetch lines
	} {3}
}

interface::test {fetch isnull 1} {
	$object exec -usefetch {select * from "person"}
	$object fetch isnull 2 2
} 1

interface::test {fetch isnull 1} {
	$object exec -usefetch {select * from "person"}
	$object fetch isnull current 2
} {line -1 out of range} 1

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
} {field 4 out of range} 1

interface::test {fetch line out of range} {
	$object exec -usefetch {select * from "person"}
	$object fetch 3
} {line 3 out of range} 1

interface::test {fetch isnull field out of range} {
	$object exec -usefetch {select * from "person"}
	$object fetch isnull 2 5
} {field 5 out of range} 1

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

interface::test {fetch with no fetch result available} {
	catch {$object fetch clear}
	$object exec {select * from "person"}
	$object fetch
} {no result available: invoke exec method with -usefetch option first} 1

interface::test {deal with error in fetch lines} {
	$object exec -usefetch {select * from "person"}
	catch {$object fetch lines}
	$object fetch
	$object fetch
} {jd John Do 17.5}

interface::test {fetch and begin/rollback} {
	$object exec -usefetch {select * from "person"}
	$object fetch
	$object begin
	$object exec {
		insert into "person"("id","first_name","name") values(10,'Try','It');
	}
	catch {$object fetch} result
	set result
} {no result available: invoke exec method with -usefetch option first}

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
} {wrong number of arguments given to exec while executing command: "select * from "person" where "name" = ?"} 1

interface::test {parameters with comments and literals} {
	$object exec {select "id",'?' /* selecting what ? */ from "person" where "name" = ? and "score" = ?} {De Rijk} 20
} {{pdr ?}}

# transactions
# ------------

$object rollback

interface::test {transactions} {
	$object exec {delete from "location";}
	$object exec {delete from "person";}
	set r1 [$object exec {select "first_name" from "person" order by "id";}]
	$object begin
	$object exec {
		insert into "person" values(1,'Peter','De Rijk',20);
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
		insert into "person" values(1,'Peter','De Rijk',20);
	}
	$object exec {
		insert into "person" values(2,'John','Doe',17.5);
		insert into "person" ("id","first_name") values(3,'Jane');
	}
	$object commit
	set r4 [$object exec {select "first_name" from "person" order by "id";}]
	list $r1 $r2 $r3 $r4
} {{} {Peter John Jane} {} {Peter John Jane}}

interface::test {autocommit error} {
	$object exec {delete from "location";}
	$object exec {delete from "person";}
	set r1 [$object exec {select "first_name" from "person";}]
	catch {$object exec {
		insert into "person" values(1,'Peter','De Rijk',20);
		insert into "person" values(2,'John','Doe','error');
		insert into "person" ("id","first_name") values(3,'Jane');
	}}
	list $r1 [$object exec {select "first_name" from "person";}]
} {{} {}}

interface::test {transactions: syntax error in exec within transaction} {
	$object exec {delete from "location";}
	$object exec {delete from "person";}
	set r1 [$object exec {select "first_name" from "person";}]
	$object begin
	catch {$object exec {
		insert into "person" values(1,'Peter','De Rijk',20);
	}} error
	catch {$object exec -error {
		insert into "person" values(2,'John','Doe',18.5);
	}} error
	set r2 [$object exec {select "first_name" from "person";}]
	$object rollback
	set r3 [$object exec {select "first_name" from "person";}]
	list $r1 $r2 $r3 [string range $error 0 22]
} {{} Peter {} {unknown option "-error"}}

interface::test {transactions: sql error in exec within transaction} {
	$object exec {delete from "location";}
	$object exec {delete from "person";}
	set r1 [$object exec {select "first_name" from "person";}]
	$object begin
	catch {$object exec {
		insert into "person" values(1,'Peter','De Rijk',20);
		insert into "person" values(2,'John','Doe','error');
		insert into "person" ("id","first_name") values(3,'Jane');
	}} error
	set r2 [$object exec {select "first_name" from "person";}]
	$object rollback
	set r3 [$object exec {select "first_name" from "person";}]
	list $r1 $r2 $r3
} {{} {} {}}

interface::test {transactions: sql error in exec within transaction, seperate calls} {
	$object exec {delete from "location";}
	$object exec {delete from "person";}
	set r1 [$object exec {select "first_name" from "person";}]
	$object begin
	catch {$object exec {
		insert into "person" values(1,'Peter','De Rijk',20);
	}} error
	catch {$object exec {
		insert into "person" values(2,'John','Doe','error');
		insert into "person" ("id","first_name") values(3,'Jane');
	}} error
	set r2 [$object exec {select "first_name" from "person";}]
	$object rollback
	set r3 [$object exec {select "first_name" from "person";}]
	list $r1 $r2 $r3
} {{} {} {}}

# serial
# ------

interface::test {serial basic} {
	$object exec {delete from "location";}
	$object exec {delete from "types"}
	catch {$object serial delete types i}
	$object serial add types i
	$object exec {insert into "types" ("d") values (20)}
	$object exec {insert into "types" ("d") values (21)}
	$object exec {select "i","d" from "types" order by "d"}
} {{2 20.0} {3 21.0}}

interface::test {serial set} {
	$object exec {delete from "types"}
	catch {$object serial delete types i}
	$object serial add types i 1
	$object exec {insert into "types" ("d") values (20)}
	$object serial set "types" i 8
	set i [$object serial set types i]
	$object exec {insert into "types" ("d") values (21)}
	list $i [$object exec {select "i","d" from "types" order by "d"}]
} {8 {{2 20.0} {9 21.0}}}

interface::test {serial overrule} {
	$object exec {delete from "types"}
	catch {$object serial delete types i}
	$object serial add types i 1
	$object exec {insert into "types" ("d") values (20)}
	$object exec {insert into "types" ("i","d") values (9,21)}
	$object exec {select "i","d" from "types" order by "d"}
} {{2 20.0} {9 21.0}}

interface::test {serial next} {
	$object exec {delete from "types"}
	catch {$object serial delete types i}
	$object serial add types i
	$object exec {insert into "types" ("d") values (20)}
	set i [$object serial next types i]
	$object exec {insert into "types" ("d") values (20)}
	list $i [$object exec {select "i" from "types" order by "d"}]
} {3 {2 4}}

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

interface::test {tables 2} {
	# there maybe other tables than these present
	array set a {address 1 location 1 person 1 types 1 use 1 v_test 1}
	set tables {}
	foreach table [$object tables] {
		if {[info exists a($table)]} {lappend tables $table}
	}
	lsort $tables
} {address location person types use v_test}

interface::test {db fields} {
	$object fields person
} {id first_name name score}

interface::test {info error db fields} {
	$object fields notexist
} {table "notexist" does not exist} 1

interface::test {table info} {
	array set a [$object info table use]
	list $a(fields) $a(type,id) $a(length,place) [array names a primary,*]
} {{id person place usetime score score2} integer 100 primary,id}

interface::test {table info} {
	array set a [$object info table types]
	set result ""
	foreach name [lsort [array names a type,*]] {
		lappend result $a($name)
	}
} {char double date float integer smallint time timestamp varchar}

interface::test {info views} {
	$object info views
} {v_test}

interface::test {info access select} {
	variable user2
	$object exec "grant select on \"person\" to \"$user2\""
	list [$object info access select $user2] [lsort [$object info access select [$object info user]]]
} {person {address location person types use v_test}}

interface::test {info access select table} {
	variable user2
	$object exec "grant select on \"person\" to \"$user2\""
	list [$object info access select $user2 person] [$object info access select [$object info user] person]
} {{id first_name name score} {id first_name name score}}

interface::test {info access insert} {
	variable user2
	$object exec "grant insert on \"person\" to \"$user2\""
	list [$object info access insert $user2] [$object info access insert [$object info user]]
} {person {address location person types use v_test}}


interface::test {info access update} {
	variable user2
	$object exec "revoke update on \"person\" from $user2"
	$object exec "grant update on \"person\" to $user2"
	list [$object info access update $user2] [$object info access update [$object info user]]
} {person {address location person types use v_test}}

interface::test {info access select table} {
	variable user2
	list [$object info access select $user2 use] [$object info access select $user2 person] [$object info access select [$object info user] person]
} {{} {id first_name name score} {id first_name name score}}

if $::interface::columnperm {
	interface::test {info access update table} {
		variable user2
		$object exec "revoke update on \"person\" from $user2"
		$object exec "grant update(\"id\",\"first_name\") on \"person\" to $user2"
		list [$object info access update $user2 person] [$object info access update [$object info user] person]
	} {{id first_name} {id first_name name score}}
}

$object close
