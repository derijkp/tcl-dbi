#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

# settings
# --------

set libfiles {lib README pkgIndex.tcl init.tcl DESCRIPTION.txt}
set shareddatafiles README
set headers {}
set libbinaries [glob *[info sharedlibextension]]
set binaries {}
set extname dbi_sqlite3

# standard
# --------
source [file join [file dir [info script]] buildtools.tcl]
install $argv

