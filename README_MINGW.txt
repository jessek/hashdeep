This file discusses how to cross-compile on Linux for Windows.

Outline of this document:

1. Installing the 64-bit cross-compiler on Ubuntu (includes pthreads)
2. Installing the 32-bit cross-compiler on Ubuntu (does not include pthreads)
3. Installing the pthreads for the 32-bit cross-compiler on Ubuntu
4. Installing on Fedora (untested)
5. Once your compilers are installed, do this!


1. Installing the 64-bit cross-compiler on Ubuntu (includes pthreads)
=====================================================================

The 64-bit bit cross-compiler is not widely available, but you can download it using the code below
  (From http://sourceforge.net/tracker/?func=detail&aid=3258887&group_id=67079&atid=516781)

$ sudo apt-get install libmpfr1ldbl
$ wget http://ppa.launchpad.net/mingw-packages/ppa/ubuntu/pool/main/w/w64-toolchain/x86-64-w64-mingw32-toolchain_1.0b+201011211643-0w2273g93970b22426p16~karmic1_amd64.deb
$ sudo dpkg -i x86-64-w64-mingw32-toolchain_1.0b+201011211643-0w2273g93970b22426p16~karmic1_amd64.deb



2. Installing the 32-bit cross-compiler on Ubuntu (does not include pthreads)
=====================================================================
Install i386-mingw32msvc with:
	# sudo apt-get install gcc-mingw32

If that doesn't work, try this:
  (From http://ubuntuforums.org/showthread.php?t=1705566)

$ wget http://ppa.launchpad.net/mingw-packages/ppa/ubuntu/pool/main/w/w64-toolchain/i686-w64-mingw32-toolchain_1.0b+201011211643-0w2273g93970b22426p16~karmic1_amd64.deb
$ sudo dpkg -i i686-w64-mingw32-toolchain_1.0b+201011211643-0w2273g93970b22426p16~karmic1_amd64.deb


3. Installing the pthreads for the 32-bit cross-compiler on Ubuntu
==================================================================
We thoughtfully include a copy of the win32 pthreads implementation.

$ tar xfv dist/pthreads-w32-2-8-0-release.tar.gz
$ cd pthreads-w32-2-8-0-release
$ make clean
$ make CROSS=i586-mingw32msvc- clean GC-static
$ sudo install libpthreadGC2.a /usr/i586-mingw32msvc/lib/
$ sudo install implement.h  need_errno.h  pthread.h sched.h  semaphore.h /usr/i586-mingw32msvc/include/



4. Installing on Fedora (untested)
==================================
On Fedora, you can install mingw32 using standard yum.
To install mingw64, follow the instructions at:
  http://fedoraproject.org/wiki/MinGW/CrossCompilerFramework
  http://fedoraproject.org/wiki/Features/Mingw-w64_cross_compiler
  http://www.advancedhpc.com/tower_servers/tower_server_products.html

$ cd /etc/yum.repos.d
$ sudo wget http://fedoraproject.org/wiki/MinGW/CrossCompilerFramework#Development_and_testing_repository
$ yum update
$ yum install mingw64-gcc mingw64-g++ mingw64-zlib

You will find some other instructions at:
  http://fedoraproject.org/wiki/MinGW/CrossCompilerFramework

$ sudo yum install mingw32-gcc-c++

5. Once your compilers are installed, do this!
==============================================
Once you have the cross-compiler in place, you should be able to do this:

$ make win32 # compiles 32-bit binaries
$ make win64 # compiles 64-bit binaries
$ make windist # compiles both 32 and 64 bit binaries, then makes the zip file with them and the docs
