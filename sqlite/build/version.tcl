#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

# standard
# --------
source [file join [file dir [info script]] buildtools.tcl]
version $argv

