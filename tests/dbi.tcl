#!/bin/sh
# the next line restarts using wish \
exec tclsh "$0" "$@"
puts "source [info script]"

package require interface
package require dbi

# Test if interfaces::dbi-0.8 is a proper interface
# We won't be testing the test method: we are not sure which
# object to use (don't know which database system is available)
interface test interface-0.8 interfaces::dbi-0.8 \
	-interface interface -version 0.8 \
	-testtest 0
