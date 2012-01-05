# This Makefile is separate from the autotools files in the
# top level directory. It is used only to clean up and
# publish files for the public web site.
#
# $Id: Makefile 252 2010-12-17 17:33:34Z jessekornblum $
#

ALL_FILES = index.html style.css \
	    changes.txt \
	    md5deep.html start-md5deep.html \
	    hashdeep.html start-hashdeep.html


USERNAME = jessekornblum,md5deep
DESTDIR  = web.sourceforge.net:htdocs/

preflight:
	rm -f *~
	grep RBF *.html *.txt

manpages:
	rman -f HTML -S -l "md5deep man page" ../md5deep/md5deep.1 > md5deep.html
	rman -f HTML -S -l "hashdeep man page" ../hashdeep/hashdeep.1 > hashdeep.html

changes:
	cp ../NEWS changes.txt

publish:
	rsync -avP -e ssh $(ALL_FILES) $(USERNAME)@$(DESTDIR)

all: manpages changes publish
