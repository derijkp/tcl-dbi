Summary:	postgresql access for Tcl
Name:		dbi_postgresql
Version:	0.8.8
Release:	1
Copyright:	BSD
Group:	Development/Languages/Tcl
Source:	dbi-0.8.8.src.tar.gz
URL: http://rrna.uia.ac.be/dbi
Packager: Peter De Rijk <derijkp@uia.ua.ac.be>
Requires: tcl >= 8.3.2 interface >= 0.8 dbi >= 0.8
Prefix: /usr
%description
 dbi is a generic SQL database interface for Tcl.
 dbi_postgresql is an implementation of the dbi interface.
 Invoking "package require dbi_postgresql" will make the command dbi_postgresql available.
 With this command, objects can be created that can connect to an postgresql database and
 that support the dbi interface.

%prep
%setup -n dbi

%build
cd postgresql/build
./configure --prefix=/usr
make clean
make

%install
cd postgresql/build
make install

%files
%doc README
%doc /usr/man/mann/dbi_postgresql.n
/usr/lib/dbi_postgresql0.8
/usr/lib/libdbi_postgresql0.8.so
