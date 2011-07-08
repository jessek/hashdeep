#!/bin/sh
#
# meta-build system. Each target is run as a different script. Options:
# normal.sh - Do a "normal" configure and make.
# win.sh    - cross-compile for windows using i386-ming32 and mingw-w64
# world.sh  - same as "normal.sh"

# I haven't been able to find a 64-bit cross-compiler for Mac.

# On Ubuntu, you need to have the following packages installed:
# apt-get install mingw32. But that just gives you 32-bit.

# Best bet for cross-compiling seems to be Fedora, which gives you
# both mingw32 and mingw64:
# http://fedoraproject.org/wiki/MinGW/CrossCompilerFramework
#
# sudo yum install mingw32-gcc-c++

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
  echo I can run the i586-mingw32msvc-g++ compiler
  ./configure --host=i586-mingw32msvc
  make
fi

if i386-mingw32-g++ -v ;
then 
  echo I can can run i386-mingw32-g++ comipler
  ./configure --host=i386-mingw32
  make
  i386-mingw32-strip src/*.exe
fi

