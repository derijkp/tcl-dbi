package require interface
proc interface::dbi-0.1 args {
	description {
		commands implementing the dbi interface connect to SQL databases, and can be used to execute
		queries, get results, etc. from a database.
	}
	commands {
		open {
			description {open the given 'database'. Extra arguments can be given, depending on the backend used}
			arguments {
				database {
					type string
					description {
						identifier of the database, what this is exactly, can depend on the implementation
					}
				}
				args {free to implementation}
			}
			returns nothing
		}
		exec {
			description {
				execute the 'sql' commands in the databases. arguments are used where the 'sql' contains ?
			}
			arguments {
				-usefetch {
					type boolean
					description {
						if set, do not return the resultset of a select query as a list. The fetch
						subcmd will be used to get the results instead.
					}
				}
				-nullvalue {
					description {
						normally nulvalues in a resultset will return empty strings in the result;
						return 'string' instead for nullvalues
					}
				}
				sql {
					description {query to be invoked}
				}
				argument {
					number *
					description {}
				}
			}
			returns {
				when a select query has been processed, and the -usefetch option is not used, 
				the subcmd returns a list of lists: each element of this list contains the
				selected values of one record (in a list).
				Otherwise nothing is returned
			}
			example {
				db exec {select "value" from "test_table" where "id" = ?} 100
			}
		}
		fetch {
			description {
				fetch one line of data from the database. This will only work after the exec subcmd has 
				been invoked with the -usefetch option. The optional arguments can be used to fetch a
				spesific line and/or field
			}
			arguments {
				-nullvalue {
					type opt_string
					description {
						normally nulvalues in a resultset will return empty strings in the result;
						return 'string' instead for nullvalues
					}
				}
				-lines {
					type opt_boolean
					description {return the number of lines in the resultset instead of the next record}
				}
				-current {
					type opt_boolean
					description {return the current instead of the next record}
				}
				-fields {
					type opt_boolean
					description {return the fields present in the resultset instead of the next record}
				}
				-clear {
					type opt_boolean
					description {clear the current resultset, do not return anything}
				}
				-isnull {
					type opt_boolean
					description {returns 1 if the current result contains a NULL value at the given line and field}
				}
				line {
					type integer
					number 0-1
					description {line to fetch instead of current}
				}
				field {
					type string
					number 0-1
					description {field to fetch}
				}
			}
			returns {
				a list of selected values in one record, unless one of the special options is used
			}
			example {
				db fetch
			}
		}
		info {}
		tables {}
		tableinfo {}
		fields {}
		close {}
		begin {}
		commit {}
		rollback {}
		destroy {}
		serial {}
	}
}

proc interface::test::dbi-0.1 {db testdatabase} {
	test $what {open and close} {
		db open $::testdatabase
		db close
	} {}
	
	test $what {open error} {
		set db [dbi $::type db]
		catch {db open xxxxx}
		db exec {select * from test}
	} {dbi object has no open database, open a connection first} 1
	
	# Open test database for further tests
	db open $::testdatabase
	
	test $what {open -user error} {
		set db [dbi $::type db]
		db open testdbi -user test -password afsg
	} {} 1
	
	test $what {create table} {
		cleandb
		createdb
	} {}
	
	test $what {create and fill table} {
		cleandb
		createdb
		filldb
		set try {}
	} {}
	
	test $what {select} {
		initdb
		db exec {select * from person}
	} {{1 Peter {De Rijk}} {2 John Do} {3 Oog {}}}
	
	test $what {select with -nullvalue} {
		initdb
		db exec -nullvalue NULL {select * from person}
	} {{1 Peter {De Rijk}} {2 John Do} {3 Oog NULL}}
	
	test $what {error} {
		initdb
		if ![catch {db exec {select try from person}} result] {
			error "test should cause an error"
		}
		regexp {^database error executing command "select try from person":.*attribute 'try' not found} $result
	} 1
	
	test $what {select fetch} {
		initdb
		db exec -usefetch {select * from person}
	} {}
	
	test $what {1 fetch} {
		initdb
		db exec -usefetch {select * from person}
		db fetch
	} {1 Peter {De Rijk}}
	
	test $what {2 fetch} {
		initdb
		db exec -usefetch {select * from person}
		db fetch
		db fetch
	} {2 John Do}
	
	test $what {3 fetch} {
		initdb
		db exec -usefetch {select * from person}
		db fetch
		db fetch
		db fetch
	} {3 Oog {}}
	
	test $what {4 fetch, end} {
		initdb
		db exec -usefetch {select * from person}
		db fetch
		db fetch
		db fetch
		db fetch
	} {line 3 out of range} 1
	
	test $what {5 fetch, end} {
		initdb
		db exec -usefetch {select * from person}
		db fetch
		db fetch
		db fetch
		catch {db fetch}
		db fetch
	} {line 3 out of range} 1
	
	initdb
	db exec -usefetch {select * from person}
	set fetchnum [supported {db fetch 1}]
	
	if $fetchnum {
	
	test $what {fetch 1} {
		initdb
		db exec -usefetch {select * from person}
		db fetch 1
	} {2 John Do}
	
	test $what {fetch 1 1} {
		initdb
		db exec -usefetch {select * from person}
		db fetch 1 1
	} {John}
	
	test $what {fetch with NULL} {
		initdb
		db exec -usefetch {select * from person}
		db fetch 2
	} {3 Oog {}}
	
	test $what {fetch with -nullvalue} {
		initdb
		db exec -usefetch {select * from person}
		db fetch -nullvalue NULL 2
	} {3 Oog NULL}
	
	test $what {fetch -lines} {
		initdb
		db exec -usefetch {select * from person}
		db fetch -lines
	} {3}
	
	test $what {fetch -isnull 1} {
		initdb
		db exec -usefetch {select * from person}
		db fetch -isnull 2 2
	} 1
	
	test $what {fetch -isnull 0} {
		initdb
		db exec -usefetch {select * from person}
		db fetch -isnull 2 1
	} 0
	
	test $what {fetch field out of range} {
		initdb
		db exec -usefetch {select * from person}
		db fetch 2 3
	} {field 3 out of range} 1
	
	test $what {fetch line out of range} {
		initdb
		db exec -usefetch {select * from person}
		db fetch 3
	} {line 3 out of range} 1
	
	test $what {fetch -isnull field out of range} {
		initdb
		db exec -usefetch {select * from person}
		db fetch -isnull 2 5
	} {field 5 out of range} 1
	
	test $what {fetch -current} {
		initdb
		db exec -usefetch {select * from person}
		db fetch 0
		db fetch -current
	} 1
	
	} else {
	# no fetch positioning
	test $what {fetch 1} {
		initdb
		db exec -usefetch {select * from person}
		set cmd {db fetch 1}
		if ![catch $cmd result] {
			error "test should cause error"
		}
		regexp {positioning for fetch not supported$} $result
	} 1
	
	test $what {fetch -lines} {
		initdb
		db exec -usefetch {select * from person}
		set cmd {db fetch -lines}
		if ![catch $cmd result] {
			error "test should cause error"
		}
		regexp {fetch -lines not supported$} $result
	} 1
	
	}
	
	test $what {fetch -fields} {
		initdb
		db exec -usefetch {select * from person}
		db fetch -fields
	} {id first_name name}
	
	test $what {fetch with no fetch result available} {
		initdb
		catch {db fetch -clear}
		db exec {select * from person}
		db fetch
	} {no result available: invoke exec method with -usefetch option first} 1
	
	db close
	
	testsummarize
	
}

proc interface::test::dbi-0.1::cleandb {} {
	upvar type type
	if {"$type" == "interbase"} {
		catch {db exec "drop table test"}
		catch {db exec "drop generator test_id_seq"}
		catch {db exec "drop table person"}
		catch {db exec "drop sequence person_id_seq"}
		catch {db exec "drop table address"}
		catch {db exec "drop sequence address_id_seq"}
		catch {db exec "drop table location"}
	} else {
		catch {db exec "drop table test"}
		catch {db exec {delete from rdb$generators where rdb$generator_name = 'ADDRESS_ID_SEQ'}}
		catch {db exec "drop sequence test_id_seq"}
		catch {db exec "drop table person"}
		catch {db exec "drop sequence person_id_seq"}
		catch {db exec "drop table address"}
		catch {db exec "drop sequence address_id_seq"}
		catch {db exec "drop table location"}
	}
}

