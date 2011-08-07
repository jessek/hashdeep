# See http://www.gnu.org/s/hello/manual/automake/EXEEXT.html

OTHERS=md5deep$(EXEEXT) sha1deep$(EXEEXT) sha256deep$(EXEEXT) tigerdeep$(EXEEXT) whirlpooldeep$(EXEEXT)

# These are for the 64-bit versions:

OTHERS64_EXE=md5deep64.exe sha1deep64.exe sha256deep64.exe tigerdeep64.exe whirlpooldeep64.exe

ALGS=md5.c md5.h sha1.c sha1.h sha256.c sha256.h whirlpool.c whirlpool.h tiger.c tiger.h


bin_PROGRAMS = hashdeep$(EXEEXT)

hashdeep_SOURCES = main.cpp hashlist.cpp multihash.cpp display.cpp \
	hash.cpp dig.cpp helpers.cpp xml.cpp xml.h files.cpp common.h main.h \
	utf8.h utf8/checked.h utf8/core.h utf8/unchecked.h \
	threadpool.h threadpool.cpp \
	${ALGS}

# This magic creates md5deep, etc from hashdeep on Windows with LN
${OTHERS}: hashdeep$(EXEEXT)
	/bin/rm -f $@ 
	ln hashdeep$(EXEEXT) $@

${OTHERS64_EXE}: hashdeep64.exe
	/bin/rm -f $@ 
	ln hashdeep64.exe $@

all: ${OTHERS}

CLEANFILES=hashdeep64.exe $(OTHERS) $(OTHERS_EXE) $(OTHERS64_EXE)

