export TCLHTTPD_DIR=/usr/local/tclhttpd
export LD_LIBRARY_PATH=/usr/local/tclhttpd/lib
$TCLHTTPD_DIR/bin/tclsh8.4

#package require Extral
package require dbi
package require dbi_postgresql
dbi postgresql db
db open test
namespace eval db {}
set db::tables [db tables]
foreach table $::db::tables {
puts $table
	db tableinfo $table ::db::tinfo_$table
#	set ::db::tinfo_${table}(sfields) [join [set ::db::tinfo_${table}(fields)] ,]
}


source tools.tcl
set ::type postgresql
set what dbi-$type
set fts(fetchnum) 1
dbi $::type db
db open $::testdatabase
db tables

db tableinfo person data
parray data

db tableinfo location data
parray data

db tableinfo pcr_product data


test $what {error} {
	initdb
	if ![catch {db exec {select try from person}} result] {
		error "test should cause an error"
	}
	regexp {^database error executing command "select try from person":.*attribute 'try' not found$} $result
} 1

source tools.tcl
set ::type odbc
set what dbi-$type
dbi $::type db
db open testdbi
catch {db exec "drop table test"}
catch {db exec "drop sequence test_id_seq"}
db exec {
	create table test (
		id serial primary key,
		first_name text,
		name text
	);
}
db exec {
	insert into test (first_name, name)
		values ('John', 'Do');
}
db exec {select * from test}
db exec -usefetch {select * from test}
db fetch

source tools.tcl
set ::type odbc
set what dbi-$type
dbi $::type db
db open testdbi
test $what {fetch -fields} {
	initdb
	db exec -usefetch {select * from person}
	db fetch -fields
} {id first_name name}
