#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

# settings
# --------

set libfiles {lib README pkgIndex.tcl}
set shareddatafiles README
set headers {}
set libbinaries {}
set binaries {}

# standard
# --------
source [file join [file dir [info script]] buildtools.tcl]
install $argv

