
CC = gcc
CFLAGS = -Wall -O3
LINK_OPT = -lm

# Where we get installed
PREFIX = /usr/local

# You can cross compile this program for Win32 using the MinGW compiler
# See README for details. Put the $PREFIX used to install MinGW in CR_BASE
# The values below are used by the developer on the test platforms

# On OS X:
CR_BASE = /Users/jessekornblum/bin/cross-tools/i386-mingw32msvc/bin

# On Linux:
#CR_BASE = /home/jessek/cross-tools/i386-mingw32msvc/bin

#---------------------------------------------------------------------
# You shouldn't need to change anything below this point
#---------------------------------------------------------------------

# This should be commented out when debugging is done
#CFLAGS += -O0 -D__DEBUG -ggdb

NAME    = md5deep
VERSION = 1.9.1

ALL_GOALS   = md5deep sha1deep sha256deep whirlpooldeep tigerdeep
ALL_ALGS    = md5,sha1,sha256,whirlpool,tiger
COMMA_GOALS = {$(ALL_ALGS)}deep

# Definitions we'll need later (and that should rarely change)
HEADER_FILES  = md5deep.h hashTable.h algorithms.h {$(ALL_ALGS)}.h
SRC  =  main.c match.c hashTable.c helpers.c dig.c files.c 
SRC +=  md5.c sha1.c hash.c cycles.c sha256.c whirlpool.c tiger.c
OBJ  =  main.o match.o helpers.o dig.o cycles.o hashTable.o
DOCS = Makefile README CHANGES $(MAN_PAGE)
WINDOC = README.txt CHANGES.txt

MAN_PAGE   = $(NAME).1
CFLAGS += -D VERSION=\"$(VERSION)\"

#---------------------------------------------------------------------
# OPERATING SYSTEM DIRECTIVES
#---------------------------------------------------------------------

all: linux

goals: $(OBJ) $(ALL_GOALS)

linux: CFLAGS += -D__LINUX -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
linux: goals

sunos: solaris
solaris: CC += -D__SOLARIS -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
solaris: goals

# There are no additional defines for OS X as _APPLE_ is defined for us.
mac: goals

unix: goals

# Common commands for compiling versions for Windows. 
# See cross and windows directives below.
win_general: LINK_OPT = -liberty
win_general: SUFFIX=.exe
win_general: goals
	$(STRIP) {$(COMMA_GOALS)}.exe

# Cross compiling from Linux to Windows. See README for more info
cross: CC = $(CR_BASE)/gcc $(RAW_FLAGS)
cross: STRIP = $(CR_BASE)/strip
cross: win_general

# See the README for information on Windows compilation
windows: STRIP = strip
windows: win_general 

#---------------------------------------------------------------------
# COMPILE THE PROGRAMS
#   This section must be updated each time you add an algorithm
#---------------------------------------------------------------------

files-tiger.o: files.c
	$(CC) -c $< -o $@
hash-tiger.o: hash.c
	$(CC) -c $< -o $@

TIGEROBJ = files-tiger.o hash-tiger.o tiger.o
tigerdeep: CC += -DTIGER
tigerdeep: $(OBJ) $(TIGEROBJ)
	$(CC) $(OBJ) $(TIGEROBJ) -o $@$(SUFFIX) $(LINK_OPT)



files-whirlpool.o: files.c
	$(CC) -c $< -o $@
hash-whirlpool.o: hash.c
	$(CC) -c $< -o $@

WHIRLPOOLOBJ = files-whirlpool.o hash-whirlpool.o whirlpool.o
whirlpooldeep: CC += -DWHIRLPOOL
whirlpooldeep: $(OBJ) $(WHIRLPOOLOBJ)
	$(CC) $(OBJ) $(WHIRLPOOLOBJ) -o $@$(SUFFIX) $(LINK_OPT)



files-md5.o: files.c
	$(CC) -c $< -o $@
hash-md5.o: hash.c
	$(CC) -c $< -o $@

MD5OBJ = files-md5.o hash-md5.o md5.o
md5deep: CC += -DMD5
md5deep: $(OBJ) $(MD5OBJ)
	$(CC) $(OBJ) $(MD5OBJ) -o $@$(SUFFIX) $(LINK_OPT)



files-sha256.o: files.c
	$(CC) -c $< -o $@
hash-sha256.o: hash.c
	$(CC) -c $< -o $@

SHA256OBJ = files-sha256.o hash-sha256.o sha256.o
sha256deep: CC += -DSHA256
sha256deep: $(OBJ) $(SHA256OBJ)
	$(CC) $(OBJ) $(SHA256OBJ) -o $@$(SUFFIX) $(LINK_OPT)



files-sha1.o: files.c
	$(CC) -c $< -o $@
hash-sha1.o: hash.c
	$(CC) -c $< -o $@
SHA1OBJ = files-sha1.o hash-sha1.o sha1.o
sha1deep: CC += -DSHA1
sha1deep: $(OBJ) $(SHA1OBJ)
	$(CC) $(OBJ) $(SHA1OBJ) -o $@$(SUFFIX) $(LINK_OPT)


#---------------------------------------------------------------------
# INSTALLATION AND REMOVAL 
#---------------------------------------------------------------------

BIN = $(PREFIX)/bin
MAN = $(PREFIX)/man/man1

install: goals
	install -d $(BIN) $(MAN)
	install -m 755 $(ALL_GOALS) $(BIN)
	install -m 644 $(MAN_PAGE) $(MAN)
	ln -fs md5deep.1 $(MAN)/sha1deep.1
	ln -fs md5deep.1 $(MAN)/sha256deep.1
	ln -fs md5deep.1 $(MAN)/whirlpooldeep.1
	ln -fs md5deep.1 $(MAN)/tigerdeep.1

uninstall:
	rm -f -- $(BIN)/{$(COMMA_GOALS)}
	rm -f -- $(MAN)/{$(COMMA_GOALS)}.1

#---------------------------------------------------------------------
# CLEAN UP
#---------------------------------------------------------------------

# Anything in the code that needs further attention is marked with the
# letters 'RBF', which stand for "Remove Before Flight." This is similar
# to how aircraft are marked with streamers to indicate maintaince is
# required before the plane can fly. Typing 'make preflight' will do a
# check for all RBF tags; thus showing the developer what needs to be
# fixed before the program can be released.
preflight:
	@grep -n RBF *.1 *.h *.c README CHANGES

nice:
	rm -f -- *~

clean: nice
	rm -f -- *.o $(ALL_GOALS) $(WIN_DOC)
	rm -f -- {$(COMMA_GOALS)}.exe
	rm -f -- $(TAR_FILE).gz $(DEST_DIR).zip $(DEST_DIR).zip.gpg

#-------------------------------------------------------------------------
# MAKING PACKAGES
#-------------------------------------------------------------------------

EXTRA_FILES = 
DEST_DIR = $(NAME)-$(VERSION)
TAR_FILE = $(DEST_DIR).tar
PKG_FILES = $(SRC) $(HEADER_FILES) $(DOCS) $(EXTRA_FILES)
ZIP_FILES = {$(COMMA_GOALS)}.exe $(WINDOC) 

# This packages me up to send to somebody else
package:
	rm -f $(TAR_FILE) $(TAR_FILE).gz
	mkdir $(DEST_DIR)
	cp $(PKG_FILES) $(DEST_DIR)
	tar cvf $(TAR_FILE) $(DEST_DIR)
	rm -rf $(DEST_DIR)
	gzip $(TAR_FILE)


# This Makefile is designed for Mac OS X to package the file. 
# To do this on a linux box, The big line below starting with "/usr/bin/tbl"
# should be replaced with:
#
#	man ./$(MAN_PAGE) | col -bx > README.txt
#
# and the "flip -d" command should be replaced with unix2dos
#
# The flip command can be found at:
# http://ccrma-www.stanford.edu/~craig/utility/flip/#

win-doc:
#	man ./$(MAN_PAGE) | col -bx > README.txt
	/usr/bin/tbl ./$(MAN_PAGE) | /usr/bin/groff -S -Wall -mtty-char -mandoc -Tascii | /usr/bin/col -bx > README.txt
	cp CHANGES CHANGES.txt
#	unix2dos $(WINDOC)
	flip -d $(WINDOC)

cross-pkg: cross win-doc
	rm -f $(DEST_DIR).zip
	zip $(DEST_DIR).zip $(ZIP_FILES)
	rm -f $(WINDOC)

world: package cross-pkg
