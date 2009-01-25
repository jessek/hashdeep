# This Makefile is separate from the autotools files in the
# top level directory. It is used only to clean up and
# publish files for the public web site.
#
# $Id$
#

USERNAME =jessekornblum
DESTDIR  =shell.sourceforge.net:/home/groups/m/md/md5deep/htdocs

RMAN = /sw/bin/rman

clean:
	rm -f *~

manpages:
	$(RMAN) -f HTML -S -l "md5deep man page" ../md5deep/md5deep.1 > md5deep.html
	$(RMAN) -f HTML -S -l "hashdeep man page" ../hashdeep/hashdeep.1 > hashdeep.html

changes:
	cp ../NEWS changes.txt

publish:
	scp index.html style.css $(USERNAME)@$(DESTDIR)

all: clean manpages changes
	scp *.html changes.txt style.css $(USERNAME)@$(DESTDIR)
