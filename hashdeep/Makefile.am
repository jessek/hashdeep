bin_PROGRAMS = hashdeep

hashdeep_SOURCES = main.cpp audit.c match.c multihash.c display.cpp hashtable.c \
	hash.c dig.c cycles.c ui.c helpers.cpp progname.c \
	md5.c sha1.c sha256.c tiger.c whirlpool.c md5deep_hashtable.c files.c xml.cpp

# If don't have strsep
# we compile it ourselves using a local copy via EXTRA_HASHDEEP

EXTRA_hashdeep_SOURCES = lib-strsep.c
hashdeep_LDADD = ${EXTRA_HASHDEEP} -lm
hashdeep_DEPENDENCIES = ${EXTRA_HASHDEEP}

EXTRA_DIST = main.h strsep.h ${man_MANS}

CLEANFILES=hashdeep64.exe
