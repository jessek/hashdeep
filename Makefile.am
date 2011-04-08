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
CROSS64_PREFIX = x86_64-w64-mingw32

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

win64: distclean
	./configure --host=$(CROSS64_PREFIX)
	make
	$(CROSS64_PREFIX)-strip md5deep/*exe hashdeep/*.exe
	mv md5deep/md5deep.exe       md5deep/md5deep64.exe
	mv md5deep/sha1deep.exe      md5deep/sha1deep64.exe
	mv md5deep/sha256deep.exe    md5deep/sha256deep64.exe
	mv md5deep/tigerdeep.exe     md5deep/tigerdeep64.exe
	mv md5deep/whirlpooldeep.exe md5deep/whirlpooldeep64.exe
	mv hashdeep/hashdeep.exe     hashdeep/hashdeep64.exe
	cp md5deep/*.exe hashdeep/*.exe $(distdir)

win32: distclean
	./configure --host=$(CROSS_PREFIX)
	make
	$(CROSS_PREFIX)-strip md5deep/*.exe hashdeep/*.exe
	cp md5deep/*.exe hashdeep/*.exe $(distdir)


win-docs: $(WINDOWSDOCS)
	$(FLIP) $(WINDOWSDOCS)
#	flip -d $(WINDOWSDOCS)
#	unix2dos $(WINDOWSDOCS)
#	todos $(WINDOWSDOCS)

win-prep:
	rm -f $(WINDOWSDOCS)

win-package: win-prep win-docs
	cp *.TXT $(distdir)

world: dist
	rm -rf $(distdir).zip $(distdir) md5deep/*.exe hashdeep/*.exe
	mkdir $(distdir)

	make win32
	make win64
	make win-package

	zip -r9 $(distdir).zip $(distdir)
	rm -rf $(distdir) $(WINDOWSDOCS) MD5DEEP.1 HASHDEEP.1

