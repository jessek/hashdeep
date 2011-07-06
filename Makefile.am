SUBDIRS = src tests man

EXTRA_DIST = FILEFORMAT config.guess config.sub  \
   testfiles/anotherfile.txt \
   testfiles/foo.txt \
   testfiles/one-byte-file \
   testfiles/somefile.doc \
   testfiles/thirdfile.xlsx \
   testfiles/zero-byte-file


nice:
	rm -f *~ man/*~ src/*~

preflight:
	grep RBF */*.{c,cpp,h,1} doc/* AUTHORS README NEWS TODO ChangeLog 

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
	cp man/md5deep.1   MD5DEEP.1
HASHDEEP.1:
	cp man/hashdeep.1 HASHDEEP.1

win64: distclean
	./configure --host=$(CROSS64_PREFIX)
	make
	$(CROSS64_PREFIX)-strip src/*exe src/*.exe
	mv src/md5deep.exe       src/md5deep64.exe
	mv src/sha1deep.exe      src/sha1deep64.exe
	mv src/sha256deep.exe    src/sha256deep64.exe
	mv src/tigerdeep.exe     src/tigerdeep64.exe
	mv src/whirlpooldeep.exe src/whirlpooldeep64.exe
	mv src/hashdeep.exe     src/hashdeep64.exe
	cp src/*.exe src/*.exe $(distdir)

win32: distclean
	./configure --host=$(CROSS_PREFIX)
	make
	$(CROSS_PREFIX)-strip src/*.exe src/*.exe
	cp src/*.exe src/*.exe $(distdir)


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
	rm -rf $(distdir).zip $(distdir) src/*.exe src/*.exe
	mkdir $(distdir)

	make win32
	make win64
	make win-package

	zip -r9 $(distdir).zip $(distdir)
	rm -rf $(distdir) $(WINDOWSDOCS) MD5DEEP.1 HASHDEEP.1

