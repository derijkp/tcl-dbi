#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

package require dbi
package require dbi_odbc

set db [dbi_odbc]
set db2 [dbi_odbc]

interface test dbi 0.1 $db \
	-testdb testdbi -user2 PDR \
	-object2 $db2

interface::opendb
interface::initdb

set interface::interface dbi_odbc
set interface::version 0.1

$db destroy
$db2 destroy

interface::testsummarize
