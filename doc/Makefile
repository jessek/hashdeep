# This Makefile is separate from the autotools files in the
# top level directory. It is used only to clean up and
# publish files for the public web site.
#
# $Id$
#

clean:
	rm -f *~

manpages:
	rman

changes:
	cp ../NEWS changes.txt

publish:
	scp index.html style.css jessekornblum@

all:
	scp *.html 