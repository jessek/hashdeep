ALL_GOALS={md5,sha1,sha256,tiger,whirlpool}

AM_LDFLAGS=-lm

bin_PROGRAMS=md5deep sha1deep sha256deep tigerdeep whirlpooldeep

base_src=main.c dig.c cycles.c hash.c ui.c helpers.c files.c \
                match.c hashTable.c \
		md5.h sha1.h sha256.h tiger.h whirlpool.h \
		main.h tchar-local.h hashTable.h

md5deep_SOURCES      =${base_src} md5.c
sha1deep_SOURCES     =${base_src} sha1.c
sha256deep_SOURCES   =${base_src} sha256.c
tigerdeep_SOURCES    =${base_src} tiger.c
whirlpooldeep_SOURCES=${base_src} whirlpool.c

# We erase the old man pages, just in case they might be symbolic links
# (symbolic links were used in a previous release)
install-exec-hook: update-manpages
	rm -f ${man1dir}/${ALL_GOALS}deep.1

update-manpages: md5deep.1
	cp $< sha1deep.1
	cp $< sha256deep.1
	cp $< tigerdeep.1
	cp $< whirlpooldeep.1

man_MANS=md5deep.1 sha1deep.1 sha256deep.1 tigerdeep.1 whirlpooldeep.1

EXTRA_DIST=${man_MANS} config.guess config.sub

nice:
	rm -f *~

preflight:
	@grep RBF *.c *.h ${man_MANS} AUTHORS README NEWS TODO ChangeLog 

todo:
	@grep TODO *.c *.h ${man_MANS} AUTHORS README NEWS ChangeLog
	@echo
	@echo From TODO file:
	@cat TODO

reconfig:
	aclocal && autoconf && autoheader && automake && ./config.status


# This Makefile is designed to package Windows distributions from Mac OS X
# To do this on a Linux box, The big line below starting with "/usr/bin/tbl"
# should be replaced with:
#
#       man ./$(MAN_PAGE) | col -bx > README.txt
#
# and the "flip -d" command should be replaced with unix2dos. Note that
# these commands are included below but are commented out.
#
# The flip command can be found at:
# http://ccrma-www.stanford.edu/~craig/utility/flip/#

WINDOC=CHANGES.TXT README.TXT

CHANGES.TXT: NEWS
	cp NEWS CHANGES.TXT
	flip -d CHANGES.TXT
#	unix2dos CHANGES.TXT

README.TXT:
	/usr/bin/tbl ./$(man_MANS) | /usr/bin/groff -S -Wall -mtty-char -mandoc -Tascii | /usr/bin/col -bx > README.TXT
	flip -d README.TXT
#	man ./md5deep.1 | col -bx > README.TXT
#	unix2dos README.TXT

cross: ${WINDOC}
	./configure --host=mingw32
	make all
	mingw32-strip ${ALL_GOALS}deep.exe
	zip -9 md5deep-${VERSION}.zip ${ALL_GOALS}deep.exe $(WINDOC)
	rm -f ${WINDOC}
	make distclean

world: dist cross

MAINTAINERCLEANFILES= md5deep-${VERSION}.zip md5deep-${VERSION}.tar.gz \
                     ${ALL_GOALS}deep.exe
