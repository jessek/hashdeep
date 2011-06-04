OTHERS=md5deep sha1deep sha256deep tigerdeep whirlpooldeep

bin_PROGRAMS = hashdeep 

hashdeep_SOURCES = main.cpp audit.c match.c multihash.c display.cpp hashtable.c \
	hash.c dig.c cycles.c ui.c helpers.cpp \
	md5.c sha1.c sha256.c tiger.c whirlpool.c md5deep_hashtable.c files.c xml.cpp \
	md5deep_match.c \
	common.h \
	main.h \
	md5.h \
	md5deep.h \
	md5deep_hashtable.h \
	sha1.h \
	sha256.h \
	strsep.h \
	tchar-local.h \
	tiger.h \
	whirlpool.h \
	xml.h 

${OTHERS}: hashdeep
	/bin/rm -f $@ ; ln hashdeep $@

all: ${OTHERS}

CLEANFILES=hashdeep64.exe $(OTHERS)

