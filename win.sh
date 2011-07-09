#!/bin/sh
#
# meta-build system. Each target is run as a different script. Options:
# normal.sh - Do a "normal" configure and make.
# win.sh    - cross-compile for windows using i386-ming32 and mingw-w64
# world.sh  - same as "normal.sh"

# On Ubuntu, you can get going in many cases with "apt-get install gcc-mingw32"
#
# If you need to install both the 64-bit and 32-bit mingw compilers, try this:
#
# (from http://sourceforge.net/tracker/?func=detail&aid=3258887&group_id=67079&atid=516781)
# For 64-bit build systems:
# apt-get install libmpfr1ldbl
# wget http://ppa.launchpad.net/mingw-packages/ppa/ubuntu/pool/main/w/w64-toolchain/x86-64-w64-mingw32-toolchain_1.0b+201011211643-0w2273g93970b22426p16~karmic1_amd64.deb
# dpkg -i x86-64-w64-mingw32-toolchain_1.0b+201011211643-0w2273g93970b22426p16~karmic1_amd64.deb

# For 32-bit build systems:
# wget http://ppa.launchpad.net/mingw-packages/ppa/ubuntu/pool/main/w/w64-toolchain/i686-w64-mingw32-toolchain_1.0b+201011211643-0w2273g93970b22426p16~karmic1_amd64.deb
# dpkg -i i686-w64-mingw32-toolchain_1.0b+201011211643-0w2273g93970b22426p16~karmic1_amd64.deb
#
# Source for this tip: http://ubuntuforums.org/showthread.php?t=1705566

# I haven't been able to find a 64-bit cross-compiler for Mac.



# On Fedora, you can install mingw32 using standard yum.
# To install mingw64, follow the instructions at:
# https://fedoraproject.org/wiki/Features/Mingw-w64_cross_compiler
# http://www.advancedhpc.com/tower_servers/tower_server_products.html
# $ cd /etc/yum.repos.d
# $ sudo wget http://build1.openftd.org/mingw-w64/mingw-w64.repo
# $ sudo yum install mingw64-runtime mingw64-zlib


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

