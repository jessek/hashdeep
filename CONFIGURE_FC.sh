#!/bin/sh
cat <<EOF
****************************************************************
Configuring FC15/16 for cross-compiling multi-threaded 32-bit and
		 64-bit Windows programs with mingw.
****************************************************************

This script will configure a fresh Fedora 15 or 16 system to compile with
mingw32 and 64.  It requires:

1. Fedora Core installed and running. Typically you will do this by:

   1a - download the ISO for the 64-bit DVD (not the live media) from:
        http://fedoraproject.org/en/get-fedora-options#formats
   1b - Create a new VM using this ISO as the boot. The ISO will
        install off of its packages on your machine.
   1c - Run Applications/Other/Users and Groups from the Applications menu.
   1d - Type user user password.
   1e - Click your username / Properties / Groups.
   1f - Add yourself to the wheel group.
   1g - Type your password again.
   1h - Close the user manager
   
   1i - Start up Terminal; Chose Terminal's 
        Edit/Profile Preferences/Scrolling and check 'Unlimited' scrollback. 
   1j - Chose Applications/System Tools/System Settings/Screen.
        Select brightness 1 hour and Uncheck lock.
       
   NOTE: The first time you log in, the system will block the yum 
   system as it downloads updates. This is annoying.

2. This script. Put it in your home directory.

3. Root access. This script must be run as root. You can do that 
   by typing:
          sudo sh CONFIGURE_FC.sh

press any key to continue...
EOF
read

if [ $USER != "root" ]; then
  echo ERROR: You must run this script as root.
  exit 1
fi

if [ ! -r /etc/redhat-release ]; then
  echo ERROR: This script requires Fedora Linux.
  echo Please download Fedora-16-x86_64-Live-Desktop.iso.
  echo Boot the ISO and chose System Tools / Install to Hard Drive.
  echo You will then need to:
  echo      sudo yum -y install subversion
  echo      svn co https://md5deep.svn.sourceforge.net/svnroot/md5deep
  echo Then run this script again.
  exit 1
fi

if grep 'Fedora.release.15' /etc/redhat-release ; then
  echo Detected Fedora Core Release 15, ok.
elif grep 'Fedora.release.16' /etc/redhat-release ; then
  echo Detected Fedora Core Release 16, good.
else
  echo
  echo ERROR: Unsupported operating system. Try using Fedora Core 16.
  exit 1
fi

echo Attempting to install tools necessary to build md5deep...
echo Installing wget...
yum -y install wget
echo Successfully installed wget.

echo Installing Fedora Win32 and Win64 cross compiler packages...
echo For information, please see:
echo http://fedoraproject.org/wiki/MinGW/CrossCompilerFramework
if [ ! -d /etc/yum.repos.d ]; then
  echo ERROR: /etc/yum.repos.d does not exist. This is very bad. I quit.
  exit 1
fi
if [ ! -r /etc/yum.repos.d/fedora-cross.repo ] ; then
  if wget --directory-prefix=/etc/yum.repos.d  http://build1.openftd.org/fedora-cross/fedora-cross.repo ; then
    echo Successfully downloaded repository list for cross compilers.
  else
    echo ERROR: Could not download the repository list cross compilers.
    exit 1
  fi
fi

echo Now adding all of the packages that we will need...
if yum -y --nogpgcheck install autoconf automake gcc gcc-c++ mingw32-gcc mingw32-gcc-c++ mingw64-gcc mingw64-gcc-c++ ; then
  echo Successfully installed all required dependencies.
else
  echo ERROR: Could not install required dependencies.
  exit 1
fi

echo 
echo Updating all yum packages. This may take a little while...
yum -y update

echo
echo Getting pthreads...
echo
if [ ! -r pthreads-w32-2-8-0-release.tar.gz ]; then
  wget ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-8-0-release.tar.gz
fi
/bin/rm -rf pthreads-w32-2-8-0-release 
tar xfz pthreads-w32-2-8-0-release.tar.gz

echo
echo Compiling pthreads...
echo
pushd pthreads-w32-2-8-0-release
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
popd
echo
echo Successfully installed pthreads.

echo ================================================================
echo ================================================================
echo
echo Installation complete!
echo You should now be able to cross compile the programs.
echo $ sh bootstrap.sh
echo $ ./configure
echo $ make world

#echo Now downloading md5deep SVN repository
#svn co https://xchatty@md5deep.svn.sourceforge.net/svnroot/md5deep
#if [ ! -d md5deep/branches/version4 ] ; then
#  echo md5deep subversion repository was not downloaded.
#  exit 1
#fi
#here=`pwd`
#cd md5deep/branches/version4
#sh bootstrap.sh
#./configure
#make windist
#mv *.zip "$here"
