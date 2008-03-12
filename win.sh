#!/bin/sh

make distclean
./configure --host=mingw32 CFLAGS="-Wall -W -O2"
make
mingw32-strip {hashdeep,md5deep}/*.exe
