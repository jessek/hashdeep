#!/bin/sh
cat <<EOF
****************************************************************
Configuring FC15 for cross-compiling multi-threaded 32-bit and
		 64-bit Windows programs with mingw.
****************************************************************

This script will configure a fresh Fedora Core14 system to compile
with mingw32 and 64.  It requires:

1. FC15 installed and running. Typically you will do this by
   downloading the ISO and running it in a new virtual machine or new
   hardware. Follow the directions and install.

2. This script. Put it in your home directory.

3. Root access. This script must be run as root.

Also recommended:

4. Disable the screen saver (not needed on a VM).

EOF
read

if [ $USER != "root" ]; then
  echo This script must be run as root
  exit 1
fi

if [ ! -r /etc/redhat-release ]; then
  echo This requires Fedora Linux
  echo Please download Fedora-15-x86_64-Live-Desktop.iso.
  echo Boot the ISO and chose System Tools / Install to Hard Drive.
  echo You will then need to:
  echo      sudo yum -y install subversion
  echo      svn co https://md5deep.svn.sourceforge.net/svnroot/md5deep
  echo Then run this script again.
  exit 1
fi

if grep 'Fedora.release.15' /etc/redhat-release ; then
  echo Fedora Release 15 detected
else
  echo This script is only tested for Fedora Release 15
  exit 1
fi

if [ $USER != "root" ]; then
  echo This script must be run as root
  exit 1
fi

echo wget is required for updating fedora
yum -y install wget

echo Adding the Fedora Win32 and Win64 packages
echo For information, please see:
echo http://fedoraproject.org/wiki/MinGW/CrossCompilerFramework
if [ ! -d /etc/yum.repos.d ]; then
  echo /etc/yum.repos.d does not exist. This is very bad.
  exit 1
fi
if [ ! -r /etc/yum.repos.d/fedora-cross.repo ] ; then
  cd /etc/yum.repos.d
  if wget http://build1.openftd.org/fedora-cross/fedora-cross.repo ; then
    echo Successfully downloaded
  else
    echo Cannot download the file.
    exit 1
  fi
fi

echo Now adding all of the packages that we will need
if yum -y install autoconf automake gcc strings mingw32-gcc mingw32-gcc-c++ mingw64-gcc mingw64-gcc-c++ ; then
  echo All yummy things installed
else
  could not install all yumming things.
  exit 1
fi

echo 
echo Now updating yum packages.
yum -y update

echo Getting pthreads
if [ ! -r pthreads-w32-2-8-0-release.tar.gz ]; then
  wget ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-8-0-release.tar.gz
fi
/bin/rm -rf pthreads-w32-2-8-0-release 
tar xfvz pthreads-w32-2-8-0-release.tar.gz
cd pthreads-w32-2-8-0-release

for CROSS in i686-w64-mingw32 x86_64-w64-mingw32 
do
  make CROSS=$CROSS- CFLAGS="-DHAVE_STRUCT_TIMESPEC -I." clean GC-static
  install implement.h need_errno.h pthread.h sched.h semaphore.h /usr/$CROSS/sys-root/mingw/include/
  if [ $? != 0 ]; then
    echo "Unable to install include files for $CROSS"
    exit 1
  fi
  install *.a /usr/$CROSS/sys-root/mingw/lib/
  if [ $? != 0 ]; then
    echo "Unable to install library for $CROSS"
    exit 1
  fi
  make clean
done
