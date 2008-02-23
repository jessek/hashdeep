
RAW_CC = gcc
RAW_FLAGS = -Wall -O2
LINK_OPT = -lm
VERSION = 1.5

# You can cross compile this program for Win32 using Linux and the 
# MinGW compiler. See the README for details. If you have already
# installed MinGW, put the location ($PREFIX) here:
CR_BASE = /Users/jessekornblum/bin/cross-tools/i386-mingw32msvc/bin

# You shouldn't need to change anything below this line
#---------------------------------------------------------------------

# This should be commented out when debugging is done
#RAW_FLAGS += -D__DEBUG -ggdb

NAME = md5deep

MD5GOAL = md5deep
SHA1GOAL = sha1deep
SHA256GOAL = sha256deep
ALL_GOALS = $(MD5GOAL) $(SHA1GOAL) $(SHA256GOAL)
RM_GOALS = $(MD5GOAL),$(SHA1GOAL),$(SHA256GOAL)
MAN_PAGES = $(MD5GOAL).1 $(SHA1GOAL).1 $(SHA256GOAL).1
RM_DOCS = $(MD5GOAL).1,$(SHA1GOAL).1,$(SHA256GOAL).1

RAW_FLAGS += -DVERSION=\"$(VERSION)\"

# Where we get installed
BIN = /usr/local/bin
MAN = /usr/local/man/man1

# Setup for compiling and cross-compiling for Windows
# The CR_ prefix refers to cross compiling from OSX to Windows
CR_CC = $(CR_BASE)/gcc
CR_OPT = $(RAW_FLAGS) -D__WIN32
CR_LINK = -liberty
CR_STRIP = $(CR_BASE)/strip
CR_MD5GOAL = md5deep.exe
CR_SHA1GOAL = sha1deep.exe
CR_SHA256GOAL = sha256deep.exe
WINCC = $(RAW_CC) $(RAW_FLAGS) -D__WIN32

# Generic "how to compile C files"
CC = $(RAW_CC) $(RAW_FLAGS) -D__UNIX
.c.o: 
	$(CC) -c $<

# Definitions we'll need later (and that should rarely change)
HEADER_FILES = $(NAME).h hashTable.h algorithms.h sha256.h md5.h sha1.h
SRC =  main.c match.c files.c hashTable.c helpers.c dig.c 
SRC += md5.c sha1.c hash.c cycles.c sha256.c
OBJ =  main.o match.o helpers.o dig.o cycles.o
DOCS = Makefile README CHANGES $(MAN_PAGES)  
WINDOC = README.txt CHANGES.txt


#---------------------------------------------------------------------
# OPERATING SYSTEM DIRECTIVES
#---------------------------------------------------------------------

all: linux

goals: $(ALL_GOALS)

linux: CC += -D__LINUX -DLARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
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
win_general: MD5GOAL = $(CR_MD5GOAL)
win_general: SHA1GOAL = $(CR_SHA1GOAL)
win_general: SHA256GOAL = $(CR_SHA256GOAL)
win_general: ALL_GOALS = $(MD5GOAL) $(SHA1GOAL) $(SHA256GOAL)
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

hashTable-md5.o: hashTable.c
	$(CC) -DMD5 -c hashTable.c -o hashTable-md5.o
files-md5.o: files.c
	$(CC) -DMD5 -c files.c -o files-md5.o
hash-md5.o: hash.c
	$(CC) -DMD5 -c hash.c -o hash-md5.o
md5.o: md5.c
	$(CC) -DMD5 -c md5.c

md5deep: $(OBJ) hash-md5.o md5.o files-md5.o hashTable-md5.o
	$(CC) $(OBJ) *md5.o -o $(MD5GOAL) $(LINK_OPT)



hashTable-sha256.o: hashTable.c
	$(CC) -DSHA256 -c hashTable.c -o hashTable-sha256.o
files-sha256.o: files.c
	$(CC) -DSHA256 -c files.c -o files-sha256.o
hash-sha256.o: hash.c
	$(CC) -DSHA256 -c hash.c -o hash-sha256.o
sha256.o: sha256.c
	$(CC) -DSHA256 -c sha256.c

sha256deep: $(OBJ) hash-sha256.o sha256.o files-sha256.o hashTable-sha256.o
	$(CC) $(OBJ) *sha256.o -o $(SHA256GOAL) $(LINK_OPT)



hashTable-sha1.o: hashTable.c
	$(CC) -DSHA1 -c hashTable.c -o hashTable-sha1.o
files-sha1.o: files.c
	$(CC) -DSHA1 -c files.c -o files-sha1.o
hash-sha1.o: hash.c
	$(CC) -DSHA1 -c hash.c -o hash-sha1.o
sha1.o:
	$(CC) -DSHA1 -c sha1.c

sha1deep: $(OBJ) hash-sha1.o sha1.o files-sha1.o hashTable-sha1.o
	$(CC) $(OBJ) *sha1.o -o $(SHA1GOAL) $(LINK_OPT)


#---------------------------------------------------------------------
# INSTALLATION AND REMOVAL 
#---------------------------------------------------------------------

install: goals
	install -m 755 $(ALL_GOALS) $(BIN)
	install -m 444 $(MAN_PAGES) $(MAN)

macinstall: BIN = /usr/bin/
macinstall: MAN = /usr/share/man/man1/
macinstall: mac install


uninstall:
	rm -f -- $(BIN)/{$(RM_GOALS)}
	rm -f -- $(MAN)/{$(RM_DOCS)}

macuninstall: BIN = /usr/bin
macuninstall: MAN = /usr/share/man/man1
macuninstall: uninstall

#---------------------------------------------------------------------
# CLEAN UP
#---------------------------------------------------------------------

# This is used for debugging
preflight:
	grep -n RBF *.1 *.h *.c README CHANGES

nice:
	rm -f -- *~

clean: nice
	rm -f -- *.o
	rm -f -- $(ALL_GOALS)
	rm -f -- $(CR_MD5GOAL) $(CR_SHA1GOAL) $(CR_SHA256GOAL) $(WINDOC)
	rm -f -- $(TAR_FILE).gz $(DEST_DIR).zip $(DEST_DIR).zip.gpg

#-------------------------------------------------------------------------
# MAKING PACKAGES
#-------------------------------------------------------------------------

EXTRA_FILES = 
DEST_DIR = $(NAME)-$(VERSION)
TAR_FILE = $(DEST_DIR).tar
PKG_FILES = $(SRC) $(HEADER_FILES) $(DOCS) $(EXTRA_FILES)

# This packages me up to send to somebody else
package: clean
	rm -f $(TAR_FILE) $(TAR_FILE).gz
	mkdir $(DEST_DIR)
	cp $(PKG_FILES) $(DEST_DIR)
	tar cvf $(TAR_FILE) $(DEST_DIR)
	rm -rf $(DEST_DIR)
	gzip $(TAR_FILE)


# This Makefile is designed for Mac OSX to package the file. 
# To do this on a linux box, The big line below starting with "/usr/bin/tbl"
# should be replaced with:
#
#	man ./$(MD5GOAL).1 | col -bx > README.txt
#
# and the "flip -d" command should be replaced with dos2unix
#
# The flip command can be found at:
# http://ccrma-www.stanford.edu/~craig/utility/flip/#
win-doc:
	/usr/bin/tbl ./$(MD5GOAL).1 | /usr/bin/groff -S -Wall -mtty-char -mandoc -Tascii | /usr/bin/col > README.txt
	cp CHANGES CHANGES.txt
	flip -d $(WINDOC)

cross-pkg: clean cross win-doc
	rm -f $(DEST_DIR).zip
	zip $(DEST_DIR).zip $(CR_MD5GOAL) $(CR_SHA1GOAL) $(CR_SHA256GOAL) $(WINDOC)
	rm -f $(WINDOC)

world: package cross-pkg
