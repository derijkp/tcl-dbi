# This file provides for an alternative loading of extensions
# based on directory.
# in order to load the given package, this file is sourced
# When this script is sourced, the variable $dir must contain the
# full path name of the xtensions directory.

namespace eval ::dbi {}
namespace eval ::dbi::interbase {}
set ::dbi::interbase::dir $dir
source [file join $dir lib init.tcl]
extension provide dbi_interbase 0.8.9
