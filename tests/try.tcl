package require Extral
set type interbase
source tools.tcl
set db [dbi $::type db]
puts "open /home/ib/test.gdb"
db open /home/ib/test.gdb -user pdr -password pdr
set table types
set field i
set db db
set args {}

db exec {delete from types}
db serial delete types i
db serial add types i
db exec {insert into types (d) values (?)} 20
db exec {insert into types (d) values (?)} 21
db exec {select i,d from types order by d}

