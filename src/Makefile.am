# No need to explicitly add $(EXEEXT) anymore.
#
# See http://www.gnu.org/s/hello/manual/automake/EXEEXT.html

# The algorithms:

ALGS=md5.c md5.h sha1.c sha1.h sha256.c sha256.h whirlpool.c whirlpool.h tiger.c tiger.h 
all_sources = $(ALGS) main.cpp hashlist.cpp multihash.cpp display.cpp \
	hash.cpp dig.cpp helpers.cpp xml.cpp xml.h files.cpp common.h main.h \
	utf8.h utf8/checked.h utf8/core.h utf8/unchecked.h \
	threadpool.h threadpool.cpp winpe.cpp winpe.h

hashdeep_SOURCES = $(all_sources)
md5deep_SOURCES = $(all_sources)
sha1deep_SOURCES = $(all_sources)
sha256deep_SOURCES = $(all_sources)
whirlpooldeep_SOURCES = $(all_sources)
tigerdeep_SOURCES = $(all_sources)

bin_PROGRAMS = hashdeep md5deep sha1deep sha256deep whirlpooldeep tigerdeep

# Yes, this is gross; it would be better to make them all with hard links.
# But this works. That didn't.
# A better approach is to define install-exec-hook and uninstall-exec-hook. See extending in the automake manual
# http://www.gnu.org/s/hello/manual/automake/Extending.html
