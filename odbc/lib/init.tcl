# $Format: "set ::dbi::version 0.$ProjectMajorVersion$"$
set ::dbi::version 0.0
set ::dbi::odbc_version 0.1
package provide dbi_odbc $::dbi::odbc_version

if {[lsearch [dbi info typesloaded] odbc] == -1} {
	_package_loadlib dbi_odbc [set ::dbi::odbc_version] $_package_dbi_odbc(library)
}

