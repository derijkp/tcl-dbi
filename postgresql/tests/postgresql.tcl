#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

package require dbi
package require dbi_postgresql

set db [dbi_postgresql]
set db2 [dbi_postgresql]

if 0 {
	set ::interface::interface dbi
	set ::interface::version 0.1
	set ::interface::testleak 0
	set ::interface::name dbi-01
	set ::interface::object $db
	set ::interface::object2 $db2
	array set ::interface::options {-testdb testdbi}
	set object $db
	set object2 $db2
	set user2 pdr
interface::opendb
interface::initdb
}

interface test dbi 0.1 $db \
	-testdb testdbi -user2 pdr \
	-lines 0 \
	-columnperm 0 \
	-object2 $db2

$db destroy
$db2 destroy

interface::testsummarize
