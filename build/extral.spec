Summary:	extra commands for Tcl
Name:		extral
Version:	1.1.10
Release:	1
Copyright:	BSD
Group:	Development/Languages/Tcl
Source:	Extral-1.1.10.src.tar.gz
URL: http://rrna.uia.ac.be/extral
Packager: Peter De Rijk <derijkp@uia.ua.ac.be>
Requires: tcl >= 8.0.4
Prefix: /usr/lib
%description
 Extral is a generally useful library which extends Tcl with a.o.:
        - extral list manipulation commands
        - extra string manipulation commands
        - array manipulation
        - atexit
        - tempfile
        - filing commands

%prep
%setup -n Extral

%build
cd src
./configure --prefix=/usr
make

%install
rm -rf /usr/lib/Extral-Linux-$RPM_PACKAGE_VERSION/ /usr/doc/extral-$RPM_PACKAGE_VERSION
cd src
make install
mkdir /usr/doc/extral-$RPM_PACKAGE_VERSION
ln -s /usr/lib/Extral-Linux-$RPM_PACKAGE_VERSION/docs /usr/doc/extral-$RPM_PACKAGE_VERSION/docs

%files
%doc README
/usr/lib/Extral-Linux-1.1.10
