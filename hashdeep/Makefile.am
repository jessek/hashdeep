OTHERS=md5deep sha1deep sha256deep tigerdeep whirlpooldeep

ALGS=md5.c sha1.c sha256.c tiger.c whirlpool.c \
	md5.h sha1.h sha256.h tiger.h whirlpool.h 

bin_PROGRAMS = hashdeep 

hashdeep_SOURCES = main.cpp audit.cpp match.cpp multihash.cpp display.cpp hashtable.cpp \
	hash.cpp dig.cpp cycles.cpp ui.cpp helpers.cpp xml.cpp 	md5deep_match.cpp \
	md5deep_hashtable.cpp files.cpp \
	common.h main.h md5deep.h md5deep_hashtable.h \
	strsep.h tchar-local.h xml.h  \
	utf8.h utf8/checked.h utf8/core.h utf8/unchecked.h \
	${ALGS}

${OTHERS}: hashdeep
	/bin/rm -f $@ ; ln hashdeep $@

all: ${OTHERS}

CLEANFILES=hashdeep64.exe $(OTHERS)

