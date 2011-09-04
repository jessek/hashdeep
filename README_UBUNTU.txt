Here is how we made the Ubuntu system for compiling 32 & 64 bit Windows versions with Mingw

1. Downloaded Ubuntu 11 ISO.
2. Created a virtual machine with VMWare Fusion using the ISO as the source.
3. Disabled screen saver (not needed on a VM)
4. We then asked apt-cache about mingw32 and mingw-w64 compilers available:

  user@ubuntu:~$ apt-cache search mingw
  libconfig++8 - parsing and manipulation of structured configuration files(C++ binding)
  libconfig++8-dev - parsing and manipulation of structured config files(C++ development)
  libconfig8 - parsing and manipulation of structured configuration files
  libconfig8-dev - parsing and manipulation of structured config files(development)
  gcc-mingw32 - The GNU Compiler Collection (cross compiler for MingW32 / MingW64)
  mingw-w64 - Minimalist GNU w64 (cross) runtime
  mingw32 - Minimalist GNU win32 (cross) compiler
  mingw32-binutils - Minimalist GNU win32 (cross) binutils
  mingw32-ocaml - OCaml cross-compiler based on mingw32
  mingw32-runtime - Minimalist GNU win32 (cross) runtime
  user@ubuntu:~$ 

5. sudo apt-get install -y mingw32 gcc-doc cpp-doc

   - This installs mingw32 in /usr/i586-mingw32msvc/.
     It also installs mingw32-binutils and mingw32-runtime. 
     It does not install pthreads.

6. Install pthreads for i586-mingw32msvc:

cd $HOME
ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-8-0-release.tar.gz
tar xfvz pthreads-w32-2-8-0-release.tar.gz
cd pthreads-w32-2-8-0-release
make CC=i586-mingw32msvc-gcc RC=i586-mingw32msvc-windres RANLIB=i586-mingw32msvc-ranlib clean GC-static

This creates libpthreadGC2.a  
Now you need to install it. Unfortunately there is no installation target for the static library, so you'll need to do it manually:

sudo install *.h /usr/i586-mingw32msvc/include/
sudo install libpthreadGC2.a /usr/i586-mingw32msvc/lib/


7. Now we will install the 64-bit mingw compiler. Reportedly the
mingw64 project is developing a cross-compiler that can produce both
32-bit and 64-bit code, but I haven't been able to get it to work.

The 64-bit bit cross-compiler is not widely available, but you can
  download it using the code below (From
  http://sourceforge.net/tracker/?func=detail&aid=3258887&group_id=67079&atid=516781)

$ sudo apt-get install libmpfr1ldbl
$ wget http://ppa.launchpad.net/mingw-packages/ppa/ubuntu/pool/main/w/w64-toolchain/x86-64-w64-mingw32-toolchain_1.0b+201011211643-0w2273g93970b22426p16~karmic1_amd64.deb
$ sudo dpkg -i x86-64-w64-mingw32-toolchain_1.0b+201011211643-0w2273g93970b22426p16~karmic1_amd64.deb


You can verify that the libraries are installed:

user@ubuntu:~$ ls -l /usr/x86_64-w64-mingw32/lib/*pthread*
-rw-r--r-- 1 root root 52492 2010-11-21 10:43 /usr/x86_64-w64-mingw32/lib/libpthread.a
-rw-r--r-- 1 root root 91284 2010-11-21 10:43 /usr/x86_64-w64-mingw32/lib/libpthreadGC2.a
user@ubuntu:~$ 

Strangely, pthreadGC2.a requires that pthreadGC2.dll to be installed,
while libpthread.a does not. (It's strange because it's larger, yet it
requires the external DLL.)
