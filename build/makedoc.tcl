#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

package require pkgtools
cd [pkgtools::startdir]
cd ..

# settings
# --------

# standard
# --------
pkgtools::makedoc
