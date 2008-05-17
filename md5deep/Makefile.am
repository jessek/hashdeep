
AM_LDFLAGS = -lm
INCLUDES   = -I@top_srcdir@/include
COMMON_DIR = @top_srcdir@/common

bin_PROGRAMS=md5deep sha1deep sha256deep tigerdeep whirlpooldeep

COMMON_SRC = \
${COMMON_DIR}/hash.c     \
${COMMON_DIR}/dig.c      \
${COMMON_DIR}/cycles.c   \
${COMMON_DIR}/ui.c       \
${COMMON_DIR}/helpers.c  

base_src=main.c files.c ${COMMON_SRC} \
                match.c hashTable.c display.c \
		main.h hashTable.h 

md5deep_SOURCES      =${base_src} ${COMMON_DIR}/md5.c md5deep.c   
sha1deep_SOURCES     =${base_src} ${COMMON_DIR}/sha1.c sha1deep.c
sha256deep_SOURCES   =${base_src} ${COMMON_DIR}/sha256.c sha256deep.c
tigerdeep_SOURCES    =${base_src} ${COMMON_DIR}/tiger.c tigerdeep.c
whirlpooldeep_SOURCES=${base_src} ${COMMON_DIR}/whirlpool.c whirlpooldeep.c


ALL_GOALS={md5,sha1,sha256,tiger,whirlpool}

# We erase the old man pages, just in case they might be symbolic links
# (symbolic links were used in a previous release)
install-man: update-manpages
	rm -f $(DESTDIR)${man1dir}/${ALL_GOALS}deep.1
	$(MAKE) install-man1

update-manpages: md5deep.1
	cp md5deep.1 sha1deep.1
	cp md5deep.1 sha256deep.1
	cp md5deep.1 tigerdeep.1
	cp md5deep.1 whirlpooldeep.1

man_MANS=md5deep.1 sha1deep.1 sha256deep.1 tigerdeep.1 whirlpooldeep.1

EXTRA_DIST=${man_MANS} 
