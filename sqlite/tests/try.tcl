set interface dbi
# $Format: "set version $ProjectMajorVersion$.$ProjectMinorVersion$"$
set version 1.0

package require interface
package require dbi
package require dbi_sqlite

set object [dbi_sqlite]
set object2 [dbi_sqlite]

array set opt [subst {
	-testdb test.db
	-user2 PDR
	-object2 $object2
}]

# $Format: "eval interface test dbi-$ProjectMajorVersion$.$ProjectMinorVersion$ $object [array get opt]"$
eval interface test dbi-1.0 $object [array get opt]

::dbi::opendb
::dbi::initdb

proc vecti {list} {
	set len [llength $list]
	list i $len [binary format i$len $list]
}

proc vlist {vect} {
	foreach {type len bin} $vect break
	binary scan $bin $type$len result
	return $result
}


interface::test {encode - decode} {
	set string abcd1234
	set encoded [dbi_sqlite_encode $string]
	set result [dbi_sqlite_decode $encoded]
	string equal $string $result
} {1}

interface::test {encode - decode with \0} {
	set string asdfasd\0asdf
	set encoded [dbi_sqlite_encode $string]
	set result [dbi_sqlite_decode $encoded]
	string equal $string $result
} 1

if 0 {	
package require Compress
catch {set string [compress [vecti {1 2 4 3456456 2}]]}
set estring [dbi_sqlite_encode $string]
vlist [uncompress [dbi_sqlite_decode $estring]]

	$object exec {create table test (value text)}
	$object exec {delete from test}
	$object exec {insert into test values (?)} $estring
	set result [lindex [$object exec -flat {select value from test}] 0]
vlist [uncompress [dbi_sqlite_decode $result]]
	string equal $string $result
} 1
}