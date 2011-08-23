/bin/rm -rf aclocal.m4
autoheader -f
touch NEWS README AUTHORS ChangeLog
touch stamp-h
aclocal
autoconf -f
automake --add-missing -c
