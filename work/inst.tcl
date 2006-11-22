# quick install (lib only)
rm -rf /home/peter/build/tca/Windows-intel/exts/dbi1.0.0
rm -rf /home/peter/build/tca/Windows-intel/exts/dbi1.0.0/lib
cp -r /home/peter/build/tca/Linux-i686/exts/dbi1.0.0/lib /home/peter/build/tca/Windows-intel/exts/dbi1.0.0/

# full compile and install linux
cd /home/peter/dev/dbi/Linux-i686
make distclean
../configure --prefix=/home/peter/tcl/dirtcl
make
rm -rf /home/peter/build/tca/Linux-i686/exts/dbi1.0.0
/home/peter/dev/dbi/build/install.tcl /home/peter/build/tca/Linux-i686/exts

# full cross-compile and install windows
cd /home/peter/dev/dbi/windows-intel
make distclean
cross-bconfigure.sh --prefix=/home/peter/tcl/win-dirtcl
cross-make.sh
rm -rf /home/peter/build/tca/Windows-intel/exts/dbi1.0.0
wine /home/peter/build/tca/Windows-intel/tclsh84.exe /home/peter/dev/dbi/build/install.tcl /home/peter/build/tca/Windows-intel/exts

# sqlite3 full compile and install linux
cd /home/peter/dev/dbi/sqlite3/Linux-i686
make distclean
../configure --prefix=/home/peter/tcl/dirtcl
make
rm -rf /home/peter/build/tca/Linux-i686/exts/dbi_sqlite3-1.0.0
/home/peter/dev/dbi/sqlite3/build/install.tcl /home/peter/build/tca/Linux-i686/exts

# sqlite3 full cross-compile and install windows
cd /home/peter/dev/dbi/sqlite3/windows-intel
make distclean
cross-bconfigure.sh --prefix=/home/peter/tcl/win-dirtcl --with-sqlite3=/home/peter/lib/win/lib --with-sqlite3include=/home/peter/lib/win/include
cross-make.sh
rm -rf /home/peter/build/tca/Windows-intel/exts/dbi_sqlite3-1.0.0
wine /home/peter/build/tca/Windows-intel/tclsh84.exe /home/peter/dev/dbi/sqlite3/build/install.tcl /home/peter/build/tca/Windows-intel/exts
/home/peter/dev/dbi/sqlite3/build/version.tcl
