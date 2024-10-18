#!/bin/bash

# This script builds the dbi_firebird extension using the Holy build box environment
# options:
# -b|-bits|--bits: 32 for 32 bits build (default 64)
# -d|-builddir|--builddir: top directory to build in (default ~/build/tcl$arch)
# -v|-tclversion|--tclversion: tcl version (default 8.5.19)
# -d|-debug|--debug: 1 to make binaries with debug info (-g) 0

# The Holy build box environment requires docker, make sure it is installed
# e.g. on ubuntu and derivatives
# sudo apt install docker.io
# Also make sure you have permission to use docker
# sudo usermod -a -G docker $USER

# stop on error
set -e

# Prepare and start docker with Holy Build box
# ============================================

script="$(readlink -f "$0")"
dir="$(dirname "$script")"
source "${dir}/start_hbb.sh"

# Parse arguments
# ===============

tclversion=8.6.14
clean=1
debug=0
while [[ "$#" -gt 0 ]]; do case $1 in
	-v|-tclversion|--tclversion) tclversion="$2"; shift;;
	-c|-clean|--clean) clean="$2"; shift;;
	-d|-debug|--debug) debug="$2"; shift;;
	*) echo "Unknown parameter: $1"; exit 1;;
esac; shift; done

# Script run within Holy Build box
# ================================

echo "Entering Holy Build Box environment"

# Activate Holy Build Box environment.
# Tk does not compile with these settings (X)
# only use HBB for glibc compat, not static libs
# source /hbb_shlib/activate

# print all executed commands to the terminal
set -x

# set up environment
# ------------------
yuminstall devtoolset-9
## use source instead of scl enable so it can run in a script
## scl enable devtoolset-9 bash
source /opt/rh/devtoolset-9/enable

sudo yum install -y wget

# locations
tcldir=/build/tcl$tclversion
tkdir=/build/tk$tclversion
dirtcldir=/build/dirtcl$tclversion-$arch
destdir=$dirtcldir/exts

## put dirtcl tclsh in PATH
#mkdir /build/bin || true
#cd /build/bin
#ln -sf $dirtcldir/tclsh8.6 .
#ln -sf $dirtcldir/tclsh8.6 tclsh
PATH=$dirtcldir:$PATH

# Build
# -----

# sqlite3
# -------

cd /build
wget https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip
unzip sqlite-amalgamation-3460100.zip
wget https://www.sqlite.org/2024/sqlite-autoconf-3460100.tar.gz
tar xvzf sqlite-autoconf-3460100.tar.gz
cd /build/sqlite-autoconf-3460100
make distclean || true
CFLAGS="-Os -DSQLITE_ENABLE_FTS3=1 -DSQLITE_ENABLE_FTS3_PARENTHESIS=1 -DSQLITE_ENABLE_RTREE=1" \
    ./configure --enable-shared --enable-static --enable-threadsafe --enable-dynamic-extensions \
        --prefix="/build/sqlite-3460100-$arch"
make
make install

# Compile
mkdir -p /io/sqlite3/linux-$arch
cd /io/sqlite3/linux-$arch
../build/version.tcl
if [ "$clean" = 1 ] ; then
    make distclean || true
    if [ "$debug" = 1 ] ; then
        ../configure --enable-symbols --disable-threads \
            --with-sqlite3include=/build/sqlite-3460100-$arch/include --with-sqlite3=/build/sqlite-3460100-$arch/lib \
            --prefix="$dirtcldir"
    else
        ../configure --disable-threads \
            --with-sqlite3include=/build/sqlite-3460100-$arch/include --with-sqlite3=/build/sqlite-3460100-$arch/lib \
            --prefix="$dirtcldir"
    fi
fi

# make
make
make install
rm -rf $dirtcldir/exts/dbi_sqlite* || true
mv $dirtcldir/lib/dbi_sqlite* $dirtcldir/exts/dbi_sqlite3-1.0.0

echo "Finished building dbi_sqlite3 extension in $builddir/dirtcl$tcl_version-$arch/exts/dbi_sqlite3-1.0.0"
