#!/bin/sh

make distclean
./configure CFLAGS="-Wall -W -O -g -ggdb"
make

