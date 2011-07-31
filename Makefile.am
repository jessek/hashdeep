SUBDIRS = src tests man testfiles

CROSS_PREFIX = i586-mingw32msvc
CROSS64_PREFIX = x86_64-w64-mingw32

EXTRA_DIST = FILEFORMAT config.guess config.sub  


nice:
	rm -f *~ man/*~ src/*~

preflight:
	grep RBF */*.{c,cpp,h,1} doc/* AUTHORS README NEWS TODO ChangeLog 

todo:
	@grep TODO *.c *.h ${man_MANS} AUTHORS README NEWS ChangeLog
	@echo
	@echo From TODO file:
	@cat TODO

win32: distclean
	./configure --host=$(CROSS_PREFIX)
	make
	$(CROSS_PREFIX)-strip src/*.exe src/*.exe
	cp src/*.exe src/*.exe $(distdir)

win64: distclean
	./configure --host=$(CROSS64_PREFIX)
	make
	$(CROSS64_PREFIX)-strip src/*exe 
	mv src/md5deep.exe       src/md5deep64.exe
	mv src/sha1deep.exe      src/sha1deep64.exe
	mv src/sha256deep.exe    src/sha256deep64.exe
	mv src/tigerdeep.exe     src/tigerdeep64.exe
	mv src/whirlpooldeep.exe src/whirlpooldeep64.exe
	mv src/hashdeep.exe     src/hashdeep64.exe
	cp src/*.exe $(distdir)


#man/hashdeep.txt: man/hashdeep.1
#man/md5deep.txt: man/md5deep.1

world: dist 
	rm -rf $(distdir).zip $(distdir) src/*.exe src/*.exe
	mkdir $(distdir)
	make win32
	make win64
	make man/hashdeep.txt man/md5deep.txt
	zip -r9 $(distdir).zip $(distdir)
	@echo Adding text files to $(distdir).zip 
	cp NEWS			$(distdir)/CHANGES.txt
	cp COPYING		$(distdir)/COPYING.txt
	cp FILEFORMAT		$(distdir)/FILEFORMAT.txt
	cp man/md5deep.txt	$(distdir)/HASHDEEP.txt
	cp man/hashdeep.txt	$(distdir)/MD5DEEP.txt
	zip --to-crlf $(distdir).zip $(distdir)/*.txt
	rm -rf $(distdir) $(WINDOWSDOCS) MD5DEEP.1 HASHDEEP.1
	echo "*************************"
	echo "*** THE WORLD IS MADE ***"
	echo ""
	ls -l $(distdir).{zip,tar.gz}
	echo ""
	@unzip -l $(distdir).zip

CLEANFILES = man/md5deep.txt man/hashdeep.txt

SUFFIXES = .txt .1

.1.txt:
	/usr/bin/tbl $< | /usr/bin/groff -S -Wall -mtty-char -mandoc -Tascii | /usr/bin/col -bx > $@

