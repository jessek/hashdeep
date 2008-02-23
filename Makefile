
RAW_CC = gcc
RAW_FLAGS = -Wall -O2
LINK_OPT = -lm
VERSION = 1.6

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
#RAW_FLAGS += -D__DEBUG -ggdb

NAME = md5deep

MD5GOAL = md5deep
SHA1GOAL = sha1deep
SHA256GOAL = sha256deep
WHIRLGOAL = whirlpooldeep
ALL_GOALS = $(MD5GOAL) $(SHA1GOAL) $(SHA256GOAL) $(WHIRLGOAL)
RM_GOALS = $(MD5GOAL),$(SHA1GOAL),$(SHA256GOAL),$(WHIRLGOAL)

MAN_PAGES = md5deep.1
RM_DOCS = md5deep.1,sha1deep.1,sha256deep.1,whirlpooldeep.1

RAW_FLAGS += -DVERSION=\"$(VERSION)\"

# Setup for compiling and cross-compiling for Windows
# The CR_ prefix refers to cross compiling from OSX to Windows
CR_CC = $(CR_BASE)/gcc
CR_OPT = $(RAW_FLAGS) -D__WIN32
CR_LINK = -liberty
CR_STRIP = $(CR_BASE)/strip
CR_MD5 = md5deep.exe
CR_SHA1 = sha1deep.exe
CR_SHA256 = sha256deep.exe
CR_WHIRL = whirlpooldeep.exe
CR_ALL_GOALS = $(CR_MD5) $(CR_SHA1) $(CR_SHA256) $(CR_WHIRL)
WINCC = $(RAW_CC) $(RAW_FLAGS) -D__WIN32

# Generic "how to compile C files"
CC = $(RAW_CC) $(RAW_FLAGS) -D__UNIX
.c.o: 
	$(CC) -c $<

# Definitions we'll need later (and that should rarely change)
HEADER_FILES = $(NAME).h hashTable.h algorithms.h
HEADER_FILES += sha256.h md5.h sha1.h whirlpool.h
SRC =  main.c match.c files.c hashTable.c helpers.c dig.c
SRC += md5.c sha1.c hash.c cycles.c sha256.c whirlpool.c
OBJ =  main.o match.o helpers.o dig.o cycles.o hashTable.o
DOCS = Makefile README CHANGES $(MAN_PAGES)
WINDOC = README.txt CHANGES.txt


#---------------------------------------------------------------------
# OPERATING SYSTEM DIRECTIVES
#---------------------------------------------------------------------

all: linux

goals: $(ALL_GOALS)

linux: CC += -D__LINUX -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
linux: goals

sunos: solaris
solaris: CC += -D__SOLARIS -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
solaris: goals

mac: CC += -D__MACOSX
mac: goals

unix: goals


# Common commands for compiling versions for Windows. 
# See cross and windows directives below.
win_general: LINK_OPT = $(CR_LINK)
win_general: MD5GOAL = $(CR_MD5)
win_general: SHA1GOAL = $(CR_SHA1)
win_general: SHA256GOAL = $(CR_SHA256)
win_general: WHIRLGOAL = $(CR_WHIRL)
win_general: ALL_GOALS = $(MD5GOAL) $(SHA1GOAL) $(SHA256GOAL) $(WHIRLGOAL)
win_general: goals
	$(STRIP) $(ALL_GOALS)

# Cross compiling from Linux to Windows. See README for more info
cross: CC = $(CR_CC) $(CR_OPT)
cross: STRIP = $(CR_STRIP)
cross: win_general

# See the README for information on Windows compilation
windows: CC = $(WINCC)
windows: STRIP = strip
windows: win_general 



#---------------------------------------------------------------------
# COMPILE THE PROGRAMS
#   This section must be updated each time you add an algorithm
#---------------------------------------------------------------------

files-whirlpool.o: files.c
	$(CC) -DWHIRLPOOL -c files.c -o files-whirlpool.o
hash-whirlpool.o: hash.c
	$(CC) -DWHIRLPOOL -c hash.c -o hash-whirlpool.o
whirlpool.o: whirlpool.c
	$(CC) -DWHIRLPOOL -c whirlpool.c

whirlpooldeep: $(OBJ) hash-whirlpool.o whirlpool.o files-whirlpool.o
	$(CC) $(OBJ) *whirlpool.o -o $(WHIRLGOAL) $(LINK_OPT)


files-md5.o: files.c
	$(CC) -DMD5 -c files.c -o files-md5.o
hash-md5.o: hash.c
	$(CC) -DMD5 -c hash.c -o hash-md5.o
md5.o: md5.c
	$(CC) -DMD5 -c md5.c

md5deep: $(OBJ) hash-md5.o md5.o files-md5.o 
	$(CC) $(OBJ) *md5.o -o $(MD5GOAL) $(LINK_OPT)



files-sha256.o: files.c
	$(CC) -DSHA256 -c files.c -o files-sha256.o
hash-sha256.o: hash.c
	$(CC) -DSHA256 -c hash.c -o hash-sha256.o
sha256.o: sha256.c
	$(CC) -DSHA256 -c sha256.c

sha256deep: $(OBJ) hash-sha256.o sha256.o files-sha256.o
	$(CC) $(OBJ) *sha256.o -o $(SHA256GOAL) $(LINK_OPT)



files-sha1.o: files.c
	$(CC) -DSHA1 -c files.c -o files-sha1.o
hash-sha1.o: hash.c
	$(CC) -DSHA1 -c hash.c -o hash-sha1.o
sha1.o:
	$(CC) -DSHA1 -c sha1.c

sha1deep: $(OBJ) hash-sha1.o sha1.o files-sha1.o
	$(CC) $(OBJ) *sha1.o -o $(SHA1GOAL) $(LINK_OPT)


#---------------------------------------------------------------------
# INSTALLATION AND REMOVAL 
#---------------------------------------------------------------------

BIN = $(PREFIX)/bin
MAN = $(PREFIX)/man/man1

install: goals
	install -d $(BIN) $(MAN)
	install -m 755 $(ALL_GOALS) $(BIN)
	install -m 644 $(MAN_PAGES) $(MAN)
	ln -fs md5deep.1 $(MAN)/sha1deep.1
	ln -fs md5deep.1 $(MAN)/sha256deep.1
	ln -fs md5deep.1 $(MAN)/whirlpooldeep.1

uninstall:
	rm -f -- $(BIN)/{$(RM_GOALS)}
	rm -f -- $(MAN)/{$(RM_DOCS)}

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
	grep -n RBF *.1 *.h *.c README CHANGES

nice:
	rm -f -- *~

clean: nice
	rm -f -- *.o $(ALL_GOALS) $(WIN_DOC)
	rm -f -- $(CR_ALL_GOALS)
	rm -f -- $(TAR_FILE).gz $(DEST_DIR).zip $(DEST_DIR).zip.gpg

#-------------------------------------------------------------------------
# MAKING PACKAGES
#-------------------------------------------------------------------------

EXTRA_FILES = 
DEST_DIR = $(NAME)-$(VERSION)
TAR_FILE = $(DEST_DIR).tar
PKG_FILES = $(SRC) $(HEADER_FILES) $(DOCS) $(EXTRA_FILES)

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
#	man ./$(MD5GOAL).1 | col -bx > README.txt
#
# and the "flip -d" command should be replaced with unix2dos
#
# The flip command can be found at:
# http://ccrma-www.stanford.edu/~craig/utility/flip/#
win-doc:
#	man ./$(MD5GOAL).1 | col -bx > README.txt
	/usr/bin/tbl ./$(MD5GOAL).1 | /usr/bin/groff -S -Wall -mtty-char -mandoc -Tascii | /usr/bin/col -bx > README.txt
	cp CHANGES CHANGES.txt
#	unix2dos $(WINDOC)
	flip -d $(WINDOC)

cross-pkg: cross win-doc
	rm -f $(DEST_DIR).zip
	zip $(DEST_DIR).zip $(CR_MD5) $(CR_SHA1) $(CR_SHA256) $(CR_WHIRL) $(WINDOC) 
	rm -f $(WINDOC)

world: package cross-pkg
