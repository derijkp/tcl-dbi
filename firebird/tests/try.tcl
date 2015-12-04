#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"

# valgrind --leak-check=full --gen-suppressions=yes --suppressions=/home/peter/.valgrind/file.supp tclsh

package require dbi
package require dbi_firebird
package require interface
#
set f [open ~/.password]
array set config_pw [split [read $f] "\n\t "]
close $f
#
catch {interface doc dbi}
set interface dbi
set version 1.0
set object [dbi_firebird db]
set object2 [dbi_firebird]
#
array set opt [subst {
	-testdb localhost:/home/ib/testdbi.gdb
	-openargs {-user test -password $config_pw(test)}
	-user2 test2
	-object2 $object2
}]
dbi::setup
# eval {$object create $opt(-testdb)} $opt(-openargs)
::dbi::opendb
if 0 {
	::dbi::cleandb
	::dbi::createdb
	::dbi::filldb
	#dbi::initdb
}
	$object exec {select * from "test"}


interface::test {DECIMAL type} {
	$object exec {drop table "test"}
	$object exec {
		create table "test" (
			"id" integer not null primary key,
			"dec" decimal(9,4),
			"dec2" decimal(18,4)
		)
	}
	$object exec {
		insert into "test"("id","dec","dec2") values(1,1.5,2.89)
	}
	$object exec {
		insert into "test"("id","dec","dec2") values(2,1.34543,1234.5678)
	}
	$object exec {
		insert into "test"("id","dec","dec2") values(3,?,?)
	} 18.9 1.58
	$object exec {select * from "test"}
} {{1 1.5 2.89} {2 1.3454 1234.5678} {3 18.8999 1.58}}

# add user
package require Extral
array set config_pw [file_read ~/.password]
proc makeuser {user {password qwerrqwt} {fname {}} {lname {}}} {
	set user [string toupper $user]
	# exec /opt/firebird/bin/gsec -user SYSDBA -password $::config::passwd(SYSDBA) -add $user -pw $password -fname $fname -lname $lname
	exec env ISC_USER=SYSDBA ISC_PASSWORD=$::config_pw(SYSDBA) /opt/firebird/bin/gsec -add $user -pw $password -fname $fname -lname $lname -database localhost:/opt/firebird/security2.fdb
}
makeuser test yapwyapw4
makeuser test2 yapwyapw4

$object exec {select * from "person"}

interface::test {error when info on closed object} {
	$object close
	set fields [$object info fields person]
} {dbi object has no open database, open a connection first} error
catch {::dbi::opendb}

interface::test {drop table not after -cache} {
	$object exec {create table "t" (i integer)}
	$object exec {insert into "t" values(1)}
	$object exec {select * from "t"}
	$object exec {drop table "t"}
	lsearch [$object tables] t
} -1

