#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

package require pkgtools
cd [pkgtools::startdir]

# settings
# --------

set shareddatafiles README
set headers {}
set binaries {}

# standard
# --------
pkgtools::uninstall $argv
