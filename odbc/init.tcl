# $Format: "set ::dbi::version 0.$ProjectMajorVersion$"$
set ::dbi::version 0.0
set ::dbi::odbc_version 0.1
package provide dbi_odbc $::dbi::odbc_version

dbi::loadlib dbi_odbc[set ::dbi::odbc_version] $::dbi::odbcdir

