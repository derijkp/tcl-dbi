Summary:	firebird access for Tcl
Name:		dbi_firebird
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
 dbi_firebird is an implementation of the dbi interface.
 Invoking "package require dbi_firebird" will make the command dbi_firebird available.
 With this command, objects can be created that can connect to an firebird database and
 that support the dbi interface.

%prep
%setup -n dbi

%build
cd firebird/build
./configure --prefix=/usr
make clean
make

%install
cd firebird/build
make install

%files
%doc README
%doc /usr/man/mann/dbi_firebird.n
/usr/lib/dbi_firebird1.0
/usr/lib/libdbi_firebird1.0.so
