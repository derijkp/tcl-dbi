Summary:	mysql access for Tcl
Name:		dbi_mysql
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
 dbi_mysql is an implementation of the dbi interface.
 Invoking "package require dbi_mysql" will make the command dbi_mysql available.
 With this command, objects can be created that can connect to an mysql database and
 that support the dbi interface.

%prep
%setup -n dbi

%build
cd mysql/build
./configure --prefix=/usr
make clean
make

%install
cd mysql/build
make install

%files
%doc README
%doc /usr/man/mann/dbi_mysql.n
/usr/lib/dbi_mysql0.8
/usr/lib/libdbi_mysql0.8.so
