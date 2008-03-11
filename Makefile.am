SUBDIRS = md5deep hashdeep

EXTRA_DIST = config.guess config.sub  \
   include/md5.h include/sha1.h include/sha256.h include/tiger.h \
   include/whirlpool.h include/common.h include/ui.h include/tchar-local.h

nice:
	rm -f *~ md5deep/*~ hashdeep/*~

preflight:
	@grep RBF ${SUBDIRS}/*.c ${SUBDIRS}/*.h ${SUBDIRS}/*.1 \
                  AUTHORS README NEWS TODO ChangeLog 

todo:
	@grep TODO *.c *.h ${man_MANS} AUTHORS README NEWS ChangeLog
	@echo
	@echo From TODO file:
	@cat TODO


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
	/usr/bin/tbl ./md5deep.1 | /usr/bin/groff -S -Wall -mtty-char -mandoc -Tascii | /usr/bin/col -bx > README.TXT
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
