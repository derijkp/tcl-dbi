#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

package require interface
package require dbi

# Test if interfaces::dbi-1.0 is a proper interface
# We won't be testing the test method: we are not sure which
# object to use (don't know which database system is available)
interface test interface-1.0 interfaces::dbi-1.0 \
	-interface interface -version 1.0 \
	-testtest 0
