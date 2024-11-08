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
source "${dir}/start_hbb3.sh"

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

# firebird
# --------

mkdir ~/tmp
cd ~/tmp

# required libs
# sudo yum install epel-release -y
sudo yum install -y libtommath
sudo yum install -y libtomcrypt
sudo yum install -y icu

firebirdversion=5.0.1
firebirdurl=https://github.com/FirebirdSQL/firebird/releases/download/v5.0.1/Firebird-5.0.1.1469-0-linux-x64.tar.gz


cd ~/tmp
if [ "$bits" = '32' ] ; then
	# wget https://github.com/FirebirdSQL/firebird/releases/download/v4.0.0/Firebird-4.0.0.2496-0.i686.tar.gz
	wget https://github.com/FirebirdSQL/firebird/releases/download/v5.0.1/Firebird-5.0.1.1469-0-linux-x86.tar.gz
	tar xvzf Firebird-5.0.1.1469-0-linux-x86.tar.gz
	cd ~/tmp/Firebird-5.0.1.1469-0-linux-x86
else
	# wget https://github.com/FirebirdSQL/firebird/releases/download/v4.0.0/Firebird-4.0.0.2496-0.amd64.tar.gz
	wget https://github.com/FirebirdSQL/firebird/releases/download/v5.0.1/Firebird-5.0.1.1469-0-linux-x64.tar.gz
	tar xvzf Firebird-5.0.1.1469-0-linux-x64.tar.gz
	cd ~/tmp/Firebird-5.0.1.1469-0-linux-x64
fi

# remove search for libncurses in install.sh (gives error, but works without)
mv install.sh install.sh.ori
grep -v LIBCURSES install.sh.ori > install.sh
chmod u+x install.sh

echo $'\n' | sudo ./install.sh || true

# Build
# -----

# Compile
mkdir -p /io/firebird/linux-$arch
cd /io/firebird/linux-$arch
../build/version.tcl
if [ "$clean" = 1 ] ; then
	make distclean || true
	if [ "$debug" = 1 ] ; then
		../configure --enable-symbols --disable-threads --prefix="$dirtcldir"
	else
		../configure --disable-threads --with-firebird=/opt/firebird/lib --prefix="$dirtcldir"
	fi
fi

# make
make
make install
rm -rf $dirtcldir/exts/dbi_firebird* || true
mv $dirtcldir/lib/dbi_firebird* $dirtcldir/exts

echo "Finished building genomecomb extension"
