Summary:	sqlite access for Tcl
Name:		dbi_sqlite
Version:	1.0.0
Release:	1
Copyright:	BSD
Group:	Development/Languages/Tcl
Source:	dbi-1.0.0.src.tar.gz
URL: http://rrna.uia.ac.be/dbi
Packager: Peter De Rijk <derijkp@uia.ua.ac.be>
Requires: tcl >= 8.3.2 interface >= 1.0 dbi >= 1.0
Prefix: /usr
%description
 dbi is a generic SQL database interface for Tcl.
 dbi_sqlite is an implementation of the dbi interface.
 Invoking "package require dbi_sqlite" will make the command dbi_sqlite available.
 With this command, objects can be created that can connect to an sqlite3 database and
 that support the dbi interface.

%prep
%setup -n dbi

%build
cd sqlite/build
./configure --prefix=/usr
make clean
make

%install
cd sqlite/build
make install

%files
%doc README
%doc /usr/man/mann/dbi_sqlite.n
/usr/lib/dbi_sqlite3.0
/usr/lib/libdbi_sqlite3.0.so
