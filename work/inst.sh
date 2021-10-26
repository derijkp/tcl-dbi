#!/bin/sh
set -ex

# $Format: "export version=$ProjectMajorVersion$.$ProjectMinorVersion$.$ProjectPatchLevel$"$
export version=1.0.0
name=dbi

# full compile and install linux
echo "---------- Full install Linux-i686 to tca ----------"
mkdir -p $HOME/dev/${name}/linux-ix86
cd $HOME/dev/${name}/linux-ix86
. $HOME/mybin/cross-compat-i686.sh
../build/version.tcl
make distclean || true
PATH=$CROSSNBIN:$PATH ../configure --prefix=$DIRTCL
PATH=$CROSSNBIN:$PATH make
rm -rf $HOME/build/tca/Linux-i686/exts/${name}$version
$HOME/tcl/dirtcl-i686/tclsh $HOME/dev/${name}/build/install.tcl $HOME/build/tca/Linux-i686/exts

# full compile and install linux 64bit
echo "---------- Full install Linux-x86_64 to tca ----------"
mkdir -p $HOME/dev/${name}/linux-x86_64
cd $HOME/dev/${name}/linux-x86_64
. $HOME/mybin/cross-compat-x86_64.sh
../build/version.tcl
make distclean || true
PATH=$CROSSNBIN:$PATH ../configure --prefix=$DIRTCL
PATH=$CROSSNBIN:$PATH make
rm -rf $HOME/build/tca/Linux-x86_64/exts/${name}$version
$HOME/tcl/dirtcl-x86_64/tclsh $HOME/dev/${name}/build/install.tcl $HOME/build/tca/Linux-x86_64/exts

# full cross-compile and install windows
echo "---------- Full install windows-intel to tca ----------"
mkdir -p $HOME/dev/${name}/win32-ix86
cd $HOME/dev/${name}/win32-ix86
. $HOME/mybin/cross-compat-i686-pc-mingw32.sh
../build/version.tcl
make distclean || true
PATH=$CROSSBIN:$PATH ../configure --prefix=$DIRTCL --host=$HOST --build=i386-linux
PATH=$CROSSBIN:$PATH make
rm -rf $HOME/build/tca/Windows-intel/exts/${name}$version
wintclsh z:$HOME/dev/${name}/build/install.tcl z:$HOME/build/tca/Windows-intel/exts
