Summary:	interbase access for Tcl
Name:		dbi_interbase
Version:	0.8.4
Release:	1
Copyright:	BSD
Group:	Development/Languages/Tcl
Source:	dbi-0.8.4.src.tar.gz
URL: http://rrna.uia.ac.be/dbi
Packager: Peter De Rijk <derijkp@uia.ua.ac.be>
Requires: tcl >= 8.3.2 interface >= 0.8 dbi >= 0.8
Prefix: /usr
%description
 dbi is a generic SQL database interface for Tcl.
 dbi_interbase is an implementation of the dbi interface.
 Invoking "package require dbi_interbase" will make the command dbi_interbase available.
 With this command, objects can be created that can connect to an interbase database and
 that support the dbi interface.

%prep
%setup -n dbi

%build
cd interbase/build
./configure --prefix=/usr
make clean
make

%install
cd interbase/build
make install

%files
%doc README
%doc /usr/man/mann/dbi_interbase.n
/usr/lib/dbi_interbase0.8
/usr/lib/libdbi_interbase0.8.so
