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
	global tcl_platform libfiles shareddatafiles headers libbinaries binaries extname
	version
	set len [llength $argv]
	if {$len == 0} {
		error "use either:\ninstall.tcl installdir\nor\ninstall.tcl pkglibdir <pkglibdir> pkgtcllibdir <pkgtcllibdir> pkgdatadir <pkgdatadir> pkgincludedir <pkgincludedir> bindir <bindir> mandir <mandir>"
	} elseif {$len == 1} {
		set config(srcdir) [file normalize [file join [file dir [info script]] ..]]
		set pkglibdir [file normalize [lindex $argv 0]]
		if {[file isdir $pkglibdir]} {
			if {[info exists extname]} {
				set pkglibdir [file join $pkglibdir $extname]
			} else {
				set pkglibdir [file join $pkglibdir [string trimright [file tail $config(srcdir)] 012345678.]]
			}
		}
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
			if {[regexp {[0-9]$} $pkglibdir]} {
				append pkglibdir -$majorversion.$minorversion.$patchlevel
			} else {
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

proc file_read {file} {
	set f [open $file]
	set c [read $f]
	close $f
	return $c
}

proc file_write {file data} {
	set f [open $file w]
	puts $f $data
	close $f
}

proc version {{argv {}}} {
	global tcl_platform libfiles shareddatafiles headers libbinaries binaries
	set len [llength $argv]
	if {$len != 0} {
		error "use:\nversion.tcl"
	}
	set srcdir [file normalize [file join [file dir [info script]] ..]]
	if {![file exists $srcdir/configure.in]} {
		error "configure.in not found in $srcdir"
	}
	set f [open $srcdir/configure.in]
	set c [read $f]
	close $f
	# regexp {\npackage ifneeded [^ ]+ ([^ ]+)} $c temp version
	regexp {MAJOR_VERSION=([^ \n]+)} $c temp majorversion
	regexp {MINOR_VERSION=([^ \n]+)} $c temp minorversion
	regexp {PATCHLEVEL=([^ \n]*)} $c temp patchlevel
	if {$patchlevel ne ""} {
		set version $majorversion.$minorversion.$patchlevel
	} else {
		set version $majorversion.$minorversion
		set patchlevel 0
	}
	if {[file exists $srcdir/DESCRIPTION.txt]} {
		set c [file_read $srcdir/DESCRIPTION.txt]
		regsub {Version: [0-9.]+} $c "Version: $version" c
		file copy -force $srcdir/DESCRIPTION.txt $srcdir/DESCRIPTION.txt~
		file_write [open $srcdir/DESCRIPTION.txt w] $c
		file delete $srcdir/DESCRIPTION.txt~
	}
	if {[file exists $srcdir/lib/init.tcl]} {
		set c [file_read $srcdir/lib/init.tcl]
		regsub {::version [0-9.]+} $c "::version $majorversion.$minorversion" c
		regsub {::patchlevel [0-9.]+} $c "::patchlevel $patchlevel" c
		file copy -force $srcdir/lib/init.tcl $srcdir/lib/init.tcl~
		file_write $srcdir/lib/init.tcl $c
		file delete $srcdir/lib/init.tcl~
	}
	if {[file exists $srcdir/pkgIndex.tcl]} {
		set c [file_read $srcdir/pkgIndex.tcl]
		regsub {package ifneeded ([^ ]+) [0-9.]+} $c "package ifneeded \\1 $majorversion.$minorversion" c
		file copy -force $srcdir/pkgIndex.tcl $srcdir/pkgIndex.tcl~
		file_write $srcdir/pkgIndex.tcl $c
		file delete $srcdir/pkgIndex.tcl~
	}
	foreach file [glob $srcdir/*.tcl] {
		set c [file_read $file]
		regsub {set Classy::appversion [0-9.]+} $c "set Classy::appversion $majorversion.$minorversion" c
		regsub {extension provide ([^ ]+) [0-9.]+} $c "extension provide \\1 $version" c
		file copy -force $file $file~
		file_write $file $c
		file delete $file~
	}
}	


