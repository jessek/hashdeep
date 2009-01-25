# This Makefile is separate from the autotools files in the
# top level directory. It is used only to clean up and
# publish files for the public web site.
#
# $Id$
#

USERNAME = jessekornblum,md5deep
DESTDIR  = web.sourceforge.net

RMAN = /sw/bin/rman

clean:
	rm -f *~

manpages:
	$(RMAN) -f HTML -S -l "md5deep man page" ../md5deep/md5deep.1 > md5deep.html
	$(RMAN) -f HTML -S -l "hashdeep man page" ../hashdeep/hashdeep.1 > hashdeep.html

changes:
	cp ../NEWS changes.txt

publish:
	sftp $(USERNAME)@$(DESTDIR)
#	put index.html
#	bye

all: clean manpages changes
	scp *.html changes.txt style.css $(USERNAME)@$(DESTDIR)
