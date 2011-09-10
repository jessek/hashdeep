#!/bin/sh
#
cat <<EOF
****************************************************************
Configuring UBUNUT for cross-compiling multi-threaded 32-bit and
		 64-bit Windows programs with mingw.
****************************************************************

This script will configure a fresh Ubuntu 11.04 system to compile
with mingw32 and 64.  It requires:

1. Ubuntu 64-bit 11.04 installed and running. Typically you will do
   this by downloading the ubuntu-11.04-desktop-amd64.iso and running
   it in a new virtual machine or new hardware. Boot the DVD and then
   click 'install Ubuntu.' 
       >> For best results, click 'download updates while installing'

2. This script. Put it in your home directory.

3. Root access. This script must be run as root.

Also recommended:

Press return to proceed....

4. Disable the screen saver (not needed on a VM).

EOF
read

if [ $USER != "root" ]; then
  echo This script must be run as root
  exit 1
fi

##
## Here's what's available to install:
##  user@ubuntu:~$ apt-cache search mingw
##  libconfig++8 - parsing and manipulation of structured configuration files(C++ binding)
##  libconfig++8-dev - parsing and manipulation of structured config files(C++ development)
##  libconfig8 - parsing and manipulation of structured configuration files
##  libconfig8-dev - parsing and manipulation of structured config files(development)
##  gcc-mingw32 - The GNU Compiler Collection (cross compiler for MingW32 / MingW64)
##  mingw-w64 - Minimalist GNU w64 (cross) runtime
##  mingw32 - Minimalist GNU win32 (cross) compiler
##  mingw32-binutils - Minimalist GNU win32 (cross) binutils
##  mingw32-ocaml - OCaml cross-compiler based on mingw32
##  mingw32-runtime - Minimalist GNU win32 (cross) runtime
##  user@ubuntu:~$ 


for package in mingw32 gcc-doc cpp-doc libmpfr1ldbl
do 
  echo Downloading and installing $package
  apt-get install -y $package
  if [ $? != 0 ]; then
    echo "Unable to load package: $package"
  fi
done

echo This installed mingw32 in /usr/i586-mingw32msvc/.
echo It also installs mingw32-binutils and mingw32-runtime. 
echo It does not install pthreads.
echo
echo Downloading and installing i586-mingw32msvc

wget ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-8-0-release.tar.gz
tar xfvz pthreads-w32-2-8-0-release.tar.gz
cd pthreads-w32-2-8-0-release
make CC=i586-mingw32msvc-gcc RC=i586-mingw32msvc-windres RANLIB=i586-mingw32msvc-ranlib clean GC-static

echo This creates libpthreadGC2.a  
echo Now we need to install it. 
echo Unfortunately there is no installation target for the static library, so 
echo we need  need to do it manually:

install *.h /usr/i586-mingw32msvc/include/
install libpthreadGC2.a /usr/i586-mingw32msvc/lib/

echo 
echo Now we will install the 64-bit mingw compiler. Reportedly the
echo mingw64 project is developing a cross-compiler that can produce both
echo 32-bit and 64-bit code, but we have not been able to get it to work.
echo 
echo The 64-bit bit cross-compiler is not widely available, but 
echo we know where to find it
# http://sourceforge.net/tracker/?func=detail&aid=3258887&group_id=67079&atid=516781
#
fn=x86-64-w64-mingw32-toolchain_1.0b+201011211643-0w2273g93970b22426p16~karmic1_amd64.deb
wget http://ppa.launchpad.net/mingw-packages/ppa/ubuntu/pool/main/w/w64-toolchain/$fn
if [ ! -r $fn ] ; then
  echo Could not download $fn. 
  echo stop
  exit 1
fi
dpkg -i $fn

echo "Verifying that the libraries are installed"
echo ls -l /usr/x86_64-w64-mingw32/lib/*pthread*
echo 
if [ ! -r /usr/x86_64-w64-mingw32/lib/libpthread.a ] ; then
  echo Something is wrong. libpthread.a did not get installed.
  exit 1
fi
