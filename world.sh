#!/bin/sh
#
# meta-build system. Each target is run as a different script. Options:
# win.sh    - cross-compile for windows using x86_64-w64-mingw32 and either i586-mingw32msvc or i386-mingw32
# world.sh  - does a make dist (for the source code distribution) and then a make win.

make distclean
./configure CFLAGS="-Wall -W -g -ggdb -O0"
make world
make dist
sh win.sh



