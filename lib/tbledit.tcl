proc tbledit {db {table {}}} {
	set w .classy__.tbledit
	set num 1
	while {[winfo exists $w$num] == 1} {incr num}
	set w $w$num
	catch {destroy $w}
	toplevel $w -bd 0 -highlightthickness 0
	wm protocol $w WM_DELETE_WINDOW "destroy $w"
	TblEditor $w.edit
	pack $w.edit -fill both -expand yes
	update
	$w.edit configure -db $db -table $table
	return $w
}
