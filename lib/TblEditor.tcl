Classy::Topframe subclass TblEditor

TblEditor method init args {
	super init
	# Create windows
	$object configure  \
		-db db
	Classy::Table $object.data \
		-rows 5 \
		-titlerows 1 \
		-roworigin -1 \
		-cols 4
	grid $object.data -row 1 -column 0 -columnspan 2 -sticky nesw
	Classy::OptionMenu $object.table 
	grid $object.table -row 0 -column 0 -sticky nesw
	$object.table set {}
	Classy::Entry $object.query \
		-label Query \
		-width 4
	grid $object.query -row 0 -column 1 -columnspan 2 -sticky nesw
	scrollbar $object.vbar
	grid $object.vbar -row 1 -column 2 -sticky nesw
	scrollbar $object.hbar \
		-orient horizontal
	grid $object.hbar -row 2 -column 0 -columnspan 2 -sticky nesw
	grid columnconfigure $object 0 -weight 1
	grid columnconfigure $object 1 -weight 1
	grid rowconfigure $object 1 -weight 1

	if {"$args" == "___Classy::Builder__create"} {return $object}
	# Parse this
	$object configure  \
		-db db
	$object.data configure \
		-xscrollcommand "$object.hbar set" \
		-command [varsubst object {$object data}] \
		-yscrollcommand "$object.vbar set"
	$object.table configure \
		-command [varsubst object {invoke value {$object configure -table $value}}] \
		-textvariable [privatevar $object table]
	$object.query configure \
		-textvariable [privatevar $object query]
	$object.vbar configure \
		-command "$object.data yview"
	$object.hbar configure \
		-command "$object.data xview"
	# Configure initial arguments
	if {"$args" != ""} {eval $object configure $args}
# ClassyTk Finalise
	Classy::todo $object refresh
	return $object
}

TblEditor addoption -db {db Db db} {
Classy::todo $object refresh
}

TblEditor addoption -table {table Table {}} {
Classy::todo $object refresh
}

TblEditor method refresh {} {
	if {![winfo ismapped $object]} {return}
	private $object options pids pid table query fields curpid
	set curpid {}
	set pids {}
	$object.data tag configure title -editable 0
	$object.data tag configure title -bg gray -font {helvetica 10 bold}
	$object.data configure -xresize 1
	set db $options(-db)
	if {![llength [info commands $db]]} return
	set tables [$db tables]
	if {[string equal [get table ""] ""]} {
		set table [lindex $tables 0]
	}
	set fields [$db fields $table]
	$object.table configure -list $tables
	array set a [$db info table $table]
	regsub {primary,} [array names a primary,*] {} pid
	if {[string equal $pid ""]} {
		set pids {}
	} else {
		if {[string equal $query ""]} {
			set querytemp ""
		} else {
			set querytemp "where $query"
		}
		set pids [$db exec [subst {select "$pid" from "$table $querytemp"}]]
	}
	set len [llength $pids]
	incr len
	putsvars object
	$object.data configure -rows $len -cols [llength $fields]
	$object.data xview 0
}

TblEditor method data {object x y args} {
	private $object fields pid pids table
	set field [lindex $fields $x]
	if {$y < 0} {
		return $field
	} else {
		set curpid [lindex $pids $y]
		return [lindex [lindex [db exec [subst {select "$field" from "$table" where "$pid" = ?}] $curpid] 0] 0]
	}
}

