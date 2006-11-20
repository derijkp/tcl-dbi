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
::dbi::initdb

set db [::dbi::sqlite3::privatedb $object]
$db exec {drop trigger _dbi_trigger_types_i}

if 0 {

# full compile and install linux
cd /home/peter/dev/dbi/sqlite3/Linux-i686
make distclean
../configure --prefix=/home/peter/tcl/dirtcl --enable-static
make
rm -rf /home/peter/build/tca/Linux-i686/exts/dbi_sqlite3-1.0.0
/home/peter/dev/dbi/sqlite3/build/install.tcl /home/peter/build/tca/Linux-i686/exts

# full cross-compile and install windows
cd /home/peter/dev/dbi/sqlite3/windows-intel
make distclean
cross-bconfigure.sh --prefix=/home/peter/tcl/win-dirtcl --enable-static --with-sqlite3=/home/peter/lib/win/lib
cross-make.sh
rm -rf /home/peter/build/tca/Windows-intel/exts/dbi_sqlite3-1.0.0
wine /home/peter/build/tca/Windows-intel/tclsh84.exe /home/peter/dev/dbi/sqlite3/build/install.tcl /home/peter/build/tca/Windows-intel/exts

}