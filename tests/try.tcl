
source tools.tcl
set ::type postgresql
set what dbi-$type
dbi $::type db
db open testdb
