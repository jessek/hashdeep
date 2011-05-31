#!/bin/sh
#
# meta-build system. Each target is run as a different script. Options:
# normal.sh - Do a "normal" configure and make.
# win.sh    - cross-compile for windows using i386-ming32
# world.sh  - same as "normal.sh"

make distclean
./configure --host=i386-mingw32 CFLAGS="-Wall -W -O2"
make
i386-mingw32-strip {hashdeep,md5deep}/*.exe
