cd ../build
set limitversion 3.0
set htmlreffile /home/peter/info/odbc/getinfo.html
set htmlreffile /data/move/odbc/getinfo.html
set dstfile ../src/odbc_getinfo.h

catch {close $f}
set f [open $htmlreffile]
catch {close $o}
set o [open $dstfile w]
puts $o {
#define STRING 1
#define BITMASK 2
#define UBITMASK 3
#define VALUE 4
#define VALUE32 5
#define ENUM 6

typedef struct Getinfo_values {
	unsigned int code;
	char *name;
} Getinfo_values;

typedef struct Getinfo_cor {
	char *name;
	unsigned int code;
	int type;
	Getinfo_values *values;
} Getinfo_cor;
}

proc skipto {f string} {
	upvar line line
	while 1 {
		if {[regexp $string $line]} break
		set line [gets $f]
	}
	return $line
}

proc extract {pattern line args} {
	foreach var $args {
		upvar $var $var
	}
	set num [regexp $pattern $line temp $args]
	if {$num != [llength $args]} {
		puts "$args error on line \"$line\""
	}
}

set line [gets $f]
skipto $f {<H2>Information Type Descriptions</H2>}
skipto $f {<TH width=50%>Returns</TH>}
skipto $f {<TR VALIGN="top">}

array set trans {
	{character string} STRING
	{SQLUSMALLINT value} VALUE
	{SQLUSMALLINT that} VALUE
	{SQLUINTEGER value} VALUE32
	{SQLINTEGER bitmask} BITMASK
	{SQLUINTEGER bitmask} UBITMASK
	{SQLUINTEGER enumerating} ENUM
}
catch {unset result}
set num 1

while 1 {
	puts $num
	# get data
	set line [gets $f]
	while 1 {
		if {[regexp {</TD>} $line temp]} break
		append line [gets $f]
	}
	set codes {}
	foreach el [split $line <] {
		if [regexp {SQL_[A-Z0-9_]+} $el code] {
			lappend codes $code
		}
	}
	extract {ODBC ([0-9.]+)} $line version
	skipto $f {>An? }
	extract {>An? ([a-zA-Z]+ [a-zA-Z]+)} $line type
	if {![info exists trans($type)]} {
		puts "Unknown type $type"
	} else {
		set type $trans($type)
	}
	set versions {}
	set values {}
	while 1 {
		set line [gets $f]
		if {[regexp {<TR VALIGN="top">} $line]} break
		if {[regexp {</table>} $line]} break
		if {[regexp {^(<P>)?(SQL_[A-Z0-9_]+)} $line temp temp value]} {
			lappend values $value
			set valueversion 0
			regexp {ODBC ([0-9.]+)} $line temp valueversion
			lappend versions $valueversion
		}
	}
	incr num
	if {$version > $limitversion} {
		puts "Skipped $code ($version)"
		continue
	}
	# write data
	if {[llength $values]} {
		set option [string tolower [string range [lindex $codes 0] 4 end]]
		puts -nonewline $o "\nGetinfo_values getinfo_values_$option\[\] = \{"
		set connect ""
		regsub {_?[A-Z0-9]*$} [lindex $values 0] {} commonval
		foreach value [lrange $values 1 end] {
			while {![string match $commonval* $value]} {
				regsub {_?[A-Z0-9]*$} $commonval {} commonval
			}
		}
		foreach value $values valueversion $versions {
			if {$valueversion > $limitversion} continue
			regsub ^${commonval}_ $value {} name
			puts -nonewline $o "$connect\n\t\{(unsigned int)$value,\"[string tolower $name]\"\}"
			set connect ,
		}
		puts $o ",\n\t\{0,NULL\},\n\}\;"
		set values getinfo_values_$option
	} else {
		set values NULL
	}
	foreach code $codes {
		set option [string tolower [string range $code 4 end]]
		if {[string equal $option driver_hdesc] || [string equal $option driver_hstmt]} continue
		set len [string length $option]
		lappend result($len) "\t\{\"$option\",$code,$type,$values\}"
	}
	if {[regexp {</table>} $line]} break
}

puts $o "\n/* ---- Codes ---- */"
set nums [lsort -integer [array names result]]
set endnum [lindex $nums end]
set parts {}
for {set num 0} {$num <= $endnum} {incr num} {
	puts $num/$endnum
	if {[info exists result($num)]} {
		puts $o "Getinfo_cor getinfo_cor$num\[\] = \{\n[join $result($num) ,\n],\n"
		puts $o "\t\{NULL,0,0,NULL\}\n\};"
		lappend parts getinfo_cor$num
	} else {
		lappend parts NULL
	}
}
puts $o "\nGetinfo_cor *getinfo_cor\[\] = \{\n\t[join $parts ,\n\t]\n\};"
puts $o "\nint getinfo_size = [expr {$endnum+1}];\n"

close $o
close $f

lsort -integer [array names list]
set total 0
foreach size [array names list] {
	incr total [expr {[llength $list($size)]/5}]
}


