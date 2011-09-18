#!/bin/sh
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

if grep 'Fedora.release.15' ; then
  echo Fedora Release 15 detected
else
  echo This script is only tested for Fedora Release 15
  exit 1
fi

if [ $USER != "root" ]; then
  echo This script must be run as root
  exit 1
fi

echo Adding the Fedora Win32 and Win64 packages
echo For information, please see:
echo http://fedoraproject.org/wiki/MinGW/CrossCompilerFramework
if [ ! -d /etc/yum.repos.d ]; then
  echo /etc/yum.repos.d does not exist. This is very bad.
  exit 1
fi
cd /etc/yum.repos.d
if wget http://build1.openftd.org/fedora-cross/fedora-cross.repo ; then
  echo Cannot download fedora-cross.repo
  exit 1
fi

echo Now adding all of the packages that we will need
yum -y install autoconf automake

echo 
echo Now updating yum packages.
yum -y update

exit 0


if [ ! -r pthreads-w32-2-8-0-release.tar.gz ]; then
  wget ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-8-0-release.tar.gz
fi
/bin/rm -rf pthreads-w32-2-8-0-release 
tar xfvz pthreads-w32-2-8-0-release.tar.gz
cd pthreads-w32-2-8-0-release

for CROSS in i686-w64-mingw32 x86_64-w64-mingw32 
do
  make CROSS=$CROSS- CFLAGS="-DHAVE_STRUCT_TIMESPEC -I." clean GC-static
  install *.h /usr/$CROSS/sys-root/mingw/include/
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
