proc copy {args} {
	puts "copy $args"
	eval file copy -force $args
}

proc del {file} {
	if {![file exists $file]} return
	puts "deleting $file"
	file delete -force $file
}

proc get {varName {default {}}} {
	if {[uplevel [list info exists $varName]]} {
		return [uplevel [list set $varName]]
	} else {
		return $default
	}
}

namespace eval package {}
proc package::architecture {} {
	global tcl_platform
	if {[string equal $tcl_platform(platform) unix]} {
		return $tcl_platform(os)-$tcl_platform(machine)
	} else {
		return $tcl_platform(platform)-$tcl_platform(machine)
	}
}

proc install {argv} {
	global tcl_platform libfiles shareddatafiles headers libbinaries binaries
	set len [llength $argv]
	if {$len == 0} {
		error "use either:\ninstall.tcl installdir\nor\ninstall.tcl pkglibdir <pkglibdir> pkgtcllibdir <pkgtcllibdir> pkgdatadir <pkgdatadir> pkgincludedir <pkgincludedir> bindir <bindir> mandir <mandir>"
	} elseif {$len == 1} {
		set config(srcdir) [file normalize [file join [file dir [info script]] ..]]
		set pkglibdir [file normalize [lindex $argv 0]]
		if {![regexp {[0-9]$} $pkglibdir]} {
			if {[file exists $config(srcdir)/configure.in]} {
				set f [open $config(srcdir)/configure.in]
				set c [read $f]
				close $f
				# regexp {\npackage ifneeded [^ ]+ ([^ ]+)} $c temp version
				regexp {MAJOR_VERSION=([^ \n]+)} $c temp majorversion
				regexp {MINOR_VERSION=([^ \n]+)} $c temp minorversion
				regexp {PATCHLEVEL=([^ \n]*)} $c temp patchlevel
				if {[file exists $config(srcdir)/lib/init.tcl]} {
					set f [open $config(srcdir)/lib/init.tcl]
					set c [read $f]
					close $f
					if {[regexp {patchlevel ([0-9]+)} $c temp temp]} {
						set patchlevel $temp
					}
				}
				append pkglibdir $majorversion.$minorversion.$patchlevel
			}
		}
		if {[file exists $pkglibdir]} {
			return -code error "\"$pkglibdir\" already exists"
		}
		set config(pkglibdir) $pkglibdir
		set config(pkgdatadir) $pkglibdir/shared
		set config(pkgincludedir) $pkglibdir/include
		set config(bindir) $pkglibdir/bin
	} else {
		array set config $argv
	}
	catch {file delete -force $config(pkglibdir)}
	file mkdir $config(pkglibdir)
	foreach file $libfiles {
		copy $config(srcdir)/$file $config(pkglibdir)
	}
	
	# install shared data files
	if {[llength $shareddatafiles]} {
		file mkdir $config(pkgdatadir)
		foreach file $shareddatafiles {
			copy $config(srcdir)/$file $config(pkgdatadir)
		}
	}
	# install headers
	if {[llength $headers]} {
		file mkdir $config(pkgincludedir)
		foreach file $headers {
			copy $file $config(pkgincludedir)
		}
	}
	# install docs
	if {[info exists config(mandir]} {
		if {[string equal $tcl_platform(platform) unix]} {
			set manfiles [glob -nocomplain $config(srcdir)/docs/man/*.n]
			if {[llength $manfiles]} {
				file mkdir $config(mandir)/mann
				eval file copy -force $manfiles $config(mandir)/mann
			}
		}
	}
	catch {file copy -force $config(srcdir)/docs $config(pkglibdir)}
	
	# install lib binaries
	if {[llength $libbinaries]} {
		set dir $config(pkglibdir)/[package::architecture]
		file mkdir $dir
puts "$libbinaries $dir"
		foreach binary $libbinaries {
			if {[file exists $binary]} {
				copy $binary $dir
				set root [file root $binary]
				if {[file exists $root.lib]} {
					copy $root.lib $dir
				}
			}
		}
	}
	# install bin binaries
	if {[llength $binaries]} {
		file mkdir $config(bindir)
		foreach binary $binaries {
			if {[file exists $config(srcdir)/$binary]} {
				copy $config(srcdir)/$binary $config(bindir)
				catch {file attributes $config(bindir)/$binary -permissions 644}
			}
		}
	}
}	


proc uninstall {argv} {
	global tcl_platform
	set libfiles [get config(libfiles) ""]
	set shareddatafiles [get config(shareddatafiles) ""]
	set headers [get config(headers) ""]
	set libbinaries [get config(libbinaries) ""]
	set binaries [get config(binaries) ""]
	array set config $argv
	# delete libfiles
	del $config(pkglibdir)
	
	# delete shared data files
	foreach file $shareddatafiles {
		del $config(pkgdatadir)/$file
	}
	
	# delete headers
	foreach file $headers {
		del $config(pkgincludedir)/$file
	}
	
	# delete docs
	foreach file [glob -nocomplain $config(srcdir)/doc/man/*.n] {
		del $config(mandir)/mann/[file tail $file]
	}
	
	# delete bin binaries
	file mkdir $config(bindir)
	foreach binary $binaries {
		del $config(bindir)/$binary
	}
}
