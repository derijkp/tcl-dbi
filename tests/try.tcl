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
cleandb
createdb
filldb
db exec -usefetch {select * from person}
db fetch
db fetch
db fetch


source tools.tcl
set ::type postgresql
set what dbi-$type
dbi $::type db
db open testdbi

test $what {fetch -current} {
	initdb
	db exec -usefetch {select * from person}
	db fetch 0
	db fetch -current
} 1

