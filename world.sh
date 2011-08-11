#!/bin/sh
#
# meta-build system. Each target is run as a different script. Options:
# world.sh  - does a make dist (for the source code distribution) and then a make win.

make distclean
./configure CFLAGS="-Wall -W -g -ggdb -O0"
make windist
make dist




