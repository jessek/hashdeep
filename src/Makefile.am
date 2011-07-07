OTHERS=md5deep sha1deep sha256deep tigerdeep whirlpooldeep
OTHERS_EXE=md5deep.exe sha1deep.exe sha256deep.exe tigerdeep.exe whirlpooldeep.exe
OTHERS64_EXE=md5deep64.exe sha1deep64.exe sha256deep64.exe tigerdeep64.exe whirlpooldeep64.exe

ALGS=md5.c sha1.c sha256.c tiger.c whirlpool.c \
	md5.h sha1.h sha256.h tiger.h whirlpool.h 

bin_PROGRAMS = hashdeep 

hashdeep_SOURCES = main.cpp hashlist.cpp multihash.cpp display.cpp \
	hash.cpp dig.cpp ui.cpp helpers.cpp xml.cpp xml.h files.cpp common.h main.h md5deep.h \
	utf8.h utf8/checked.h utf8/core.h utf8/unchecked.h \
	${ALGS}

# This magic creates md5deep, etc from hashdeep on Windows with LN
${OTHERS}: hashdeep
	/bin/rm -f $@ ; ln hashdeep $@

${OTHERS_EXE}: hashdeep.exe
	/bin/rm -f $@ ; ln hashdeep.exe $@

${OTHERS64_EXE}: hashdeep64.exe
	/bin/rm -f $@ ; ln hashdeep64.exe $@

all: ${OTHERS}

CLEANFILES=hashdeep64.exe $(OTHERS) $(OTHERS_EXE_

