#!/bin/sh

# $Format: "export version=$ProjectMajorVersion$.$ProjectMinorVersion$.$ProjectPatchLevel$"$
export version=1.0.0

# quick install (lib only)
rm -rf /home/peter/build/tca/Windows-intel/exts/dbi$version
rm -rf /home/peter/build/tca/Windows-intel/exts/dbi$version/lib
cp -r /home/peter/build/tca/Linux-i686/exts/dbi$version/lib /home/peter/build/tca/Windows-intel/exts/dbi$version/

# full compile and install linux
cd /home/peter/dev/dbi/Linux-i686
make distclean
../configure --prefix=/home/peter/tcl/dirtcl
make
rm -rf /home/peter/build/tca/Linux-i686/exts/dbi$version
/home/peter/dev/dbi/build/install.tcl /home/peter/build/tca/Linux-i686/exts

# full cross-compile and install windows
cd /home/peter/dev/dbi/windows-intel
make distclean
cross-bconfigure.sh --prefix=/home/peter/tcl/win-dirtcl
cross-make.sh
rm -rf /home/peter/build/tca/Windows-intel/exts/dbi$version
wine /home/peter/build/tca/Windows-intel/tclsh84.exe /home/peter/dev/dbi/build/install.tcl /home/peter/build/tca/Windows-intel/exts
