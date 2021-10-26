#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

proc tmml2html {src dst} {
	set f [open $src]
	set data [read $f]
	close $f
	regexp {<name>(.*?)</name>} $data temp title
	regexp {<desc>(.*?)</desc>} $data temp desc
	regsub {^.*</namesection>} $data {} data
	regsub -all {<ref>(.*?)</ref>} $data {<a href="\1.html">\1</a>} data
	set data [string map {
		</manpage> {}
		<section> {} </section> {}
		<title> <h2> </title> </h2>
		<seealso> {<h2>SEE ALSO</h2><ul>}
		</seealso> </ul>
		<keywords> {<h2>KEYWORDS</h2><ul>}
		</keywords> </ul>
		<keyword> <li>
		</keyword> </li>
		<commandlist> <dl> </commandlist> </dl> <commanddef> {} </commanddef> {}
		<command> <dt> </command> </dt> <desc> <dd> </desc> </dd>
		<cmd> <i> </cmd> </i> <m> <b> </m> </b>
		<dle> {} </dle> {}
		<optlist> <dl> </optlist> </dl> <optdef> {} </optdef> {}
		<optname> <dt><b> </optname> {</b> } <optarg> {} </optarg> </dt>
		<method> <i> </method> </i> <example> <pre> </example> </pre>
		$ \\$ \[ \\\[ \] \\\]
	} $data]
#	set data "\[header \"$title\"\]$data\[footer\]"
	set f [open $dst w]
	puts $f $data
	close $f
}

cd [file dir [info script]]
cd ..

foreach file [glob -nocomplain docs/xml/*.xml] {
	set destfile [file join docs man [file tail [file root $file]]].n
	if {![file exists $destfile] || ([file mtime $file] > [file mtime $destfile])} {
		puts "creating $destfile"
		if {[catch {exec tmml-man.tcl $file > $destfile} result]} {
			puts error:$result
		}
	}
	set destfile [file join docs html [file tail [file root $file].html]]
	if {![file exists $destfile] || ([file mtime $file] > [file mtime $destfile])} {
		puts "creating $destfile"
		if {[catch {tmml2html $file $destfile} result]} {
			puts error:$result
		}
	}
}
