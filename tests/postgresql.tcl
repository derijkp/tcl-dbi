#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

package require interface
package require dbi

interface list


dbi_postgresql db
interface test dbi 0.1 db -testdb testdbi
