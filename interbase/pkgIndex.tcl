# Tcl package index file, version 1.0
# This file is sourced either when an application starts up or
# by a "package unknown" script.  It invokes the
# "package ifneeded" command to set up package-related
# information so that packages will be loaded automatically
# in response to "package require" commands.  When this
# script is sourced, the variable $dir must contain the
# full path name of this file's directory.

# $Format: "package ifneeded dbi_interbase 0.$ProjectMajorVersion$ \\"$
package ifneeded dbi_interbase 0.0 \
	"[list array set _package_dbi_interbase [list execdir $dir dir {@TCLLIBDIR@} bindir {@BINDIR@} datadir {@DATADIR@} libdir {@LIBDIR@} library {@LIB_LIBRARY@}]] ; \
	source [file join $dir lib init.tcl]"
