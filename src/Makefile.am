# See http://www.gnu.org/s/hello/manual/automake/EXEEXT.html

HASHDEEP=hashdeep$(EXEEXT)
OTHERS=md5deep$(EXEEXT) sha1deep$(EXEEXT) sha256deep$(EXEEXT) tigerdeep$(EXEEXT) whirlpooldeep$(EXEEXT)

# These are for the 64-bit versions:

OTHERS64_EXE=md5deep64.exe sha1deep64.exe sha256deep64.exe tigerdeep64.exe whirlpooldeep64.exe

ALGS=md5.c md5.h sha1.c sha1.h sha256.c sha256.h whirlpool.c whirlpool.h tiger.c tiger.h


bin_PROGRAMS = $(HASHDEEP) $(OTHERS)

hashdeep_SOURCES = main.cpp hashlist.cpp multihash.cpp display.cpp \
	hash.cpp dig.cpp helpers.cpp xml.cpp xml.h files.cpp common.h main.h \
	utf8.h utf8/checked.h utf8/core.h utf8/unchecked.h \
	threadpool.h threadpool.cpp \
	${ALGS}

# Due to a bug in autoconf, each of these need to be listed explicitly.
md5deep$(EXEEXT): $(HASHDEEP)
	/bin/rm -f $@
	ln $(HASHDEEP) $@

sha1deep$(EXEEXT): $(HASHDEEP)
	/bin/rm -f $@
	ln $(HASHDEEP) $@

sha256deep$(EXEEXT): $(HASHDEEP)
	/bin/rm -f $@
	ln $(HASHDEEP) $@

tigerdeep$(EXEEXT): $(HASHDEEP)
	/bin/rm -f $@
	ln $(HASHDEEP) $@

whirlpooldeep$(EXEEXT): $(HASHDEEP)
	/bin/rm -f $@
	ln $(HASHDEEP) $@

${OTHERS64_EXE}: hashdeep64.exe
	/bin/rm -f $@ 
	ln hashdeep64.exe $@

all: ${OTHERS}

CLEANFILES=hashdeep64.exe $(OTHERS) $(OTHERS64_EXE)

