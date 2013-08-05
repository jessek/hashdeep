#!/bin/sh
cat <<EOF
****************************************************************
Configuring FC15 for cross-compiling multi-threaded 32-bit and
		 64-bit Windows programs with mingw.
****************************************************************

This script will configure a fresh Fedora system to compile with
mingw32 and 64.  It requires:

1. Fedora installed and running. Typically you will do this by:

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
          sudo bash CONFIGURE_FC.sh

press any key to continue...
EOF
read

if [ $USER != "root" ]; then
  echo ERROR: You must run this script as root.
  exit 1
fi

if grep 'Fedora.release.' /etc/redhat-release ; then
  echo Fedora Release detected
else
  echo This script is only tested for Fedora Release 18 and should work on F18 or newer.
  exit 1
fi

echo Attempting to install both DLL and static version of all mingw libraries
echo At this point we will keep going even if there is an error...
INST="autoconf automake gcc gc-c++ "
for M in mingw32 mingw64 ; do
  # For these install both DLL and static
  for lib in zlib gettext boost cairo pixman freetype fontconfig \
      bzip2 expat pthreads libgnurx libxml2 iconv openssl ; do
    INST+=" ${M}-${lib} ${M}-${lib}-static"
  done
done 
sudo yum -y install $INST

echo 
echo Updating all yum packages. This may take a little while...
yum -y update

echo ================================================================
echo ================================================================
echo
echo Installation complete!
echo You should now be able to cross compile the programs.
echo $ sh bootstrap.sh
echo $ ./configure
echo $ make world

