Summary:	sql dbms interface commands for Tcl
Name:		dbi
Version:	0.1.5
Release:	1
Copyright:	BSD
Group:	Development/Languages/Tcl
Source:	dbi-0.1.5.src.tar.gz
URL: http://rrna.uia.ac.be/dbi
Packager: Peter De Rijk <derijkp@uia.ua.ac.be>
Requires: tcl >= 8.0.4
Prefix: /usr
%description
 tcl dbi provides generic access to sql dbms's via different possible backends.
 dbi comes with 2 backends: one for odbc and one for postgresql

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
rm -rf /usr/doc/dbi-$RPM_PACKAGE_VERSION
mkdir /usr/doc/dbi-$RPM_PACKAGE_VERSION
ln -s /usr/lib/dbi0.1/docs /usr/doc/dbi-$RPM_PACKAGE_VERSION/docs

%files
%doc README
/usr/lib/dbi-0.1
/usr/lib/libdbi0.1.so
