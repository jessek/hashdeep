#!/bin/sh
#
# meta-build system. Each target is run as a different script. Options:
# normal.sh - Do a "normal" configure and make.
# win.sh    - cross-compile for windows using i386-ming32 and mingw-w64
# world.sh  - same as "normal.sh"

# On Linux, you need to have the following packages installed:
# apt-get install mingw32

if [ ! -r configure ];
then
  echo configure does not exist. Running autoreconf.
  autoreconf || ("echo automake not installed"; exit 1)
fi

if [ -r Makefile ];
then
  echo Removing old Makefile
  make distclean
fi

if i586-mingw32msvc-g++ -v ;
then 
  echo I can run i586-mingw32msvc-g++
  ./configure --host=i586-mingw32msvc
  make
fi

#echo Making 32-bit version with i386-mingw32

# First make the regular version

# ./configure --host=i386-mingw32 CFLAGS="-Wall -W -O2"
# make
# i386-mingw32-strip {hashdeep,md5deep}/*.exe
