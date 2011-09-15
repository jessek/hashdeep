if [ $USER != "root" ]; then
  echo This script must be run as root
  exit 1
fi


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
