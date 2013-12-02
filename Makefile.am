SUBDIRS = src tests man tests/testfiles

# These prefixes are for compiling on FC15
CROSS32_PREFIX = i686-w64-mingw32
CROSS64_PREFIX = x86_64-w64-mingw32

# on Mac, you may want this instead:
# CROSS_PREFIX = i386-mingw32

EXTRA_DIST = FILEFORMAT config.guess config.sub  m4/ax_pthread.m4 CONFIGURE_FC.sh

nice:
	rm -f *~ man/*~ src/*~ doc/*~

preflight:
	grep RBF */*.{c,cpp,h,1} doc/* AUTHORS README NEWS TODO ChangeLog 

todo:
	@grep TODO *.c *.h ${man_MANS} AUTHORS README NEWS ChangeLog
	@echo
	@echo From TODO file:
	@cat TODO

win32: 
	make distclean
	./configure --host=$(CROSS32_PREFIX) 
	make
	/bin/rm -f /tmp/strings.txt
	strings src/md5deep.exe > /tmp/strings.txt
	if grep pthreadGC2.dll /tmp/strings.txt ; then echo not statically linked ; exit 1; fi

win64: 
	make distclean
	./configure --host=$(CROSS64_PREFIX) 
	make
	/bin/rm -f /tmp/strings.txt
	strings src/md5deep.exe > /tmp/strings.txt
	if grep pthreadGC2.dll /tmp/strings.txt ; then echo not statically linked ; exit 1; fi
	mv src/md5deep.exe       src/md5deep64.exe
	mv src/sha1deep.exe      src/sha1deep64.exe
	mv src/sha256deep.exe    src/sha256deep64.exe
	mv src/tigerdeep.exe     src/tigerdeep64.exe
	mv src/whirlpooldeep.exe src/whirlpooldeep64.exe
	mv src/hashdeep.exe     src/hashdeep64.exe

strip32:
	$(CROSS32_PREFIX)-strip src/*.exe src/*.exe
strip64:
	$(CROSS64_PREFIX)-strip src/*64.exe 

windist: 
	@echo 
	@echo Making win32 
	@echo
	rm -rf $(distdir).zip $(distdir) src/*.exe
	mkdir $(distdir)
	make win32 
	make strip32 
	cp src/*.exe $(distdir)
	@echo ====================================
	@echo 
	@echo Making win32 
	make win64
	make strip64
	make man/hashdeep.txt man/md5deep.txt
	cp src/*.exe $(distdir)
	@echo ====================================
	@echo 
	@echo Creating ZIP archive
	@echo 
	zip -r9 $(distdir).zip $(distdir)
	@echo Adding text files to $(distdir).zip 
	cp NEWS			$(distdir)/CHANGES.txt
	cp COPYING		$(distdir)/COPYING.txt
	cp FILEFORMAT		$(distdir)/FILEFORMAT.txt
	cp man/md5deep.txt	$(distdir)/MD5DEEP.txt
	cp man/hashdeep.txt	$(distdir)/HASHDEEP.txt
	rm man/md5deep.txt man/hashdeep.txt
	zip --to-crlf $(distdir).zip $(distdir)/*.txt
	rm -rf $(distdir) $(WINDOWSDOCS) MD5DEEP.1 HASHDEEP.1
	@echo "*************************"
	@echo "*** THE WORLD IS MADE ***"
	@echo ""
	ls -l $(distdir).*
	@echo ""
	@unzip -l $(distdir).zip

wintest:
	@echo Will create md5deep-wintest.zip, a zip file that
	@echo provides for full testing on Microsoft Windows with Cygwin installed.
	make windist
	zip md5deep-wintest.zip -9 $(distdir).zip tests/* tests/*.zip tests/*.sh tests/testfiles/ tests/testfiles/*txt tests/testfiles/*/*txt tests/testfiles/*/*/*txt

world:
	@echo meta-build system. 
	@echo Making both Linux and Windows distributions
	make distclean
	./configure CFLAGS="-Wall -W -g -ggdb -O0"
	make dist
	make windist

CLEANFILES = man/md5deep.txt man/hashdeep.txt
SUFFIXES = .txt .1
.1.txt:
	man $< > $@
