# configure sqlite3 with all goodies: add
# -DSQLITE_ENABLE_FTS3=1 -DSQLITE_ENABLE_RTREE=1

# full compile and install linux
cd /home/peter/dev/dbi/sqlite3/Linux-i686
../build/version.tcl
make distclean
../configure --prefix=/home/peter/tcl/dirtcl --enable-static
make
rm -rf /home/peter/build/tca/Linux-i686/exts/dbi_sqlite3-1.0.0/
/home/peter/dev/dbi/sqlite3/build/install.tcl /home/peter/build/tca/Linux-i686/exts

# full cross-compile and install windows
cd /home/peter/dev/dbi/sqlite3/windows-intel
make distclean
#cross-bconfigure.sh --prefix=/home/peter/tcl/win-dirtcl --with-sqlite3=/home/peter/lib/win/lib --with-sqlite3include=/home/peter/lib/win/include --enable-static
cross-bconfigure.sh --prefix=/home/peter/tcl/win-dirtcl --with-sqlite3=/home/peter/lib/win/lib --enable-static
cross-make.sh
rm -rf /home/peter/build/tca/Windows-intel/exts/dbi_sqlite3-1.0.0
wine /home/peter/build/tca/Windows-intel/tclsh84.exe /home/peter/dev/dbi/sqlite3/build/install.tcl /home/peter/build/tca/Windows-intel/exts

/home/peter/dev/dbi/sqlite3/build/version.tcl
