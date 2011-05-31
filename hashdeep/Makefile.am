COMMON_DIR = @top_srcdir@/common

bin_PROGRAMS = hashdeep

hashdeep_SOURCES = hashdeep.cpp audit.c match.c multihash.c display.c hashtable.c \
	hash.c dig.c cycles.c ui.c helpers.c progname.c \
	md5.c sha1.c sha256.c tiger.c whirlpool.c 

man_MANS = hashdeep.1

# The math library is required for floor(), hence -lm
# We use strsep do our string processing, but some operating systems
# don't have it yet (e.g. OpenSolaris). We if don't have strsep
# we compile it ourselves using a local copy via EXTRA_HASHDEEP

EXTRA_hashdeep_SOURCES = lib-strsep.c
hashdeep_LDADD = ${EXTRA_HASHDEEP} -lm
hashdeep_DEPENDENCIES = ${EXTRA_HASHDEEP}

EXTRA_DIST = main.h strsep.h ${man_MANS}

CLEANFILES=hashdeep64.exe
