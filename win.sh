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
#
# For 32-bit build systems:
# wget http://ppa.launchpad.net/mingw-packages/ppa/ubuntu/pool/main/w/w64-toolchain/i686-w64-mingw32-toolchain_1.0b+201011211643-0w2273g93970b22426p16~karmic1_amd64.deb
# dpkg -i i686-w64-mingw32-toolchain_1.0b+201011211643-0w2273g93970b22426p16~karmic1_amd64.deb
#
# Source for this tip: http://ubuntuforums.org/showthread.php?t=1705566

# On Fedora, you can install mingw32 using standard yum.
# To install mingw64, follow the instructions at:
# http://fedoraproject.org/wiki/MinGW/CrossCompilerFramework
# http://fedoraproject.org/wiki/Features/Mingw-w64_cross_compiler
# http://www.advancedhpc.com/tower_servers/tower_server_products.html
#
# $ cd /etc/yum.repos.d
# $  sudo wget http://fedoraproject.org/wiki/MinGW/CrossCompilerFramework#Development_and_testing_repository
# $ yum update
# $ yum install mingw64-gcc mingw64-g++ mingw64-zlib

# Best bet for cross-compiling seems to be Fedora, which gives you
# both mingw32 and mingw64:
# http://fedoraproject.org/wiki/MinGW/CrossCompilerFramework
#
# sudo yum install mingw32-gcc-c++

# I haven't been able to find a 64-bit cross-compiler for Mac.



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

echo Trying to make the 32-bit windows
have32='no'
for cc in i586-mingw32msvc  i386-mingw32
do
  if [ $have32 = 'no' ] && ${cc}-g++ -v ;
  then 
    echo I can run the ${cc}-g++ compiler
    ./configure --host=$cc
    make
    ${cc}-strip */*.exe
    have32='yes'
  fi
done

echo Trying to make the 64-bit windows
have64='no'
for cc in x86_64-w64-mingw32
do
  if [ $have64 = 'no' ] && ${cc}-g++ -v ;
  then 
    echo I can run the ${cc}-g++ compiler
    ./configure --host=$cc
    make
    ${cc}-strip */*.exe
    have64='yes'
done
echo Successfully compiled 32-bit windows code: $have32
echo Successfully compiled 64-bit windows code: $have64

