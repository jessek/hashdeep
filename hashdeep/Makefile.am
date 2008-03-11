

AM_LDFLAGS = -lm
INCLUDES   = -I@top_srcdir@/include
COMMON_DIR = @top_srcdir@/common

bin_PROGRAMS = hashdeep

COMMON_SRC = \
${COMMON_DIR}/hash.c     \
${COMMON_DIR}/dig.c      \
${COMMON_DIR}/cycles.c   \
${COMMON_DIR}/ui.c       \
${COMMON_DIR}/helpers.c  \
${COMMON_DIR}/md5.c      \
${COMMON_DIR}/sha1.c     \
${COMMON_DIR}/sha256.c   \
${COMMON_DIR}/tiger.c    \
${COMMON_DIR}/whirlpool.c 

hashdeep_SOURCES = main.c audit.c match.c multihash.c display.c \
                   hashtable.c ${COMMON_SRC} ${EXTRA_HASHDEEP}

EXTRA_hashdeep_SOURCES = lib-strsep.c

EXTRA_DIST = main.h