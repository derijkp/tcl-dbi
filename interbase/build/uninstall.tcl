#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

# settings
# --------

set shareddatafiles README
set headers {}
set binaries {}

# standard
# --------
source [file join [file dir [info script]] buildtools.tcl]
uninstall $argv
