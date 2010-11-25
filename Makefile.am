SUBDIRS = md5deep hashdeep

EXTRA_DIST = FILEFORMAT config.guess config.sub  \
   include/md5.h include/sha1.h include/sha256.h include/tiger.h \
   include/whirlpool.h include/common.h include/ui.h include/tchar-local.h

nice:
	rm -f *~ md5deep/*~ hashdeep/*~

preflight:
	grep RBF md5deep/*.{c,h,1} hashdeep/*.{c,h,1} include/*.h common/*.c \
                 doc/* AUTHORS README NEWS TODO ChangeLog 

todo:
	@grep TODO *.c *.h ${man_MANS} AUTHORS README NEWS ChangeLog
	@echo
	@echo From TODO file:
	@cat TODO


# The flip command can be found at:
# http://ccrma-www.stanford.edu/~craig/utility/flip/#

FLIP = todos
CROSS_PREFIX = i586-mingw32msvc

SUFFIXES = .TXT .1

.1.TXT:
	/usr/bin/tbl $< | /usr/bin/groff -S -Wall -mtty-char -mandoc -Tascii | /usr/bin/col -bx > $@

#.1.TXT: 
#	man $< | col -bx > $@


WINDOWSDOCS = MD5DEEP.TXT HASHDEEP.TXT CHANGES.TXT COPYING.TXT FILEFORMAT.TXT

COPYING.TXT: COPYING
	cp COPYING COPYING.TXT

CHANGES.TXT: NEWS
	cp NEWS CHANGES.TXT

FILEFORMAT.TXT: FILEFORMAT
	cp FILEFORMAT FILEFORMAT.TXT

MD5DEEP.1: 
	cp md5deep/md5deep.1   MD5DEEP.1
HASHDEEP.1:
	cp hashdeep/hashdeep.1 HASHDEEP.1

win-docs: $(WINDOWSDOCS)
	$(FLIP) $(WINDOWSDOCS)
#	flip -d $(WINDOWSDOCS)
#	unix2dos $(WINDOWSDOCS)
#	todos $(WINDOWSDOCS)

win-prep: 
	rm -f $(WINDOWSDOCS)

win-package: win-prep win-docs
	rm -rf $(distdir).zip $(distdir)
	$(CROSS_PREFIX)-strip md5deep/*.exe hashdeep/hashdeep.exe
	mkdir $(distdir)
	cp $(WINDOWSDOCS) md5deep/*.exe hashdeep/hashdeep.exe $(distdir)
	zip -r9 $(distdir).zip $(distdir)
	rm -rf $(distdir) $(WINDOWSDOCS) MD5DEEP.1 HASHDEEP.1


world: distclean $(WINDOWSDOCS)
	./configure --host=$(CROSS_PREFIX)
	make
	make win-package
	make dist
