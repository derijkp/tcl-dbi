Summary:	sql dbms interface commands for Tcl
Name:		dbi
Version:	1.0.0
Release:	1
Copyright:	BSD
Group:	Development/Languages/Tcl
Source:	dbi-1.0.0.src.tar.gz
URL: http://tcl-dbi.sourceforge.net/
Packager: Peter De Rijk <Peter.DeRijk@ua.ac.be>
Requires: tcl >= 8.3.2 interface >= 1.0
Prefix: /usr
%description
 The dbi interface is a generic Tcl interface for accessing different SQL databases.
 It presents a generic api to open, query, and change databases.
 The dbi package contains the definition of the dbi interface, and some tools.

%prep
%setup -n dbi

%build
cd build
./configure --prefix=/usr
make clean
make

%install
cd build
make install

%files
%doc README
%doc /usr/man/mann/dbi.n
%doc /usr/man/mann/interface_dbi.n
%doc /usr/man/mann/interface_dbi_admin.n
/usr/lib/dbi1.0
