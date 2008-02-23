
RAW_CC = gcc
RAW_FLAGS = -Wall -O2
LINK_OPT = -lm
VERSION = 1.3

# You can cross compile this program for Win32 using Linux and the 
# MinGW compiler. See the README for details. If you have already
# installed MinGW, put the location ($PREFIX) here:
CROSSBASE = /home/jessek/bin/cross-tools

# You shouldn't need to change anything below this line
#---------------------------------------------------------------------

# This should be commented out when debugging is done
#RAW_FLAGS += -D__DEBUG -ggdb

NAME = md5deep
MD5GOAL = md5deep
SHA1GOAL = sha1deep

RAW_FLAGS += -DVERSION=\"$(VERSION)\"

# Where we get installed
BIN = /usr/local/bin
MAN = /usr/local/man/man1

# Setup for compiling and cross-compiling for Windows
CROSSCC = $(CROSSBASE)/i386-mingw32msvc/bin/gcc
CROSSOPT = $(RAW_FLAGS) -D__WIN32
CROSSLINK = -liberty
CROSSMD5GOAL = md5deep.exe
CROSSSHA1GOAL = sha1deep.exe
WINCC = $(RAW_CC) $(RAW_FLAGS) -D__WIN32

# Generic "how to compile C files"
CC = $(RAW_CC) $(RAW_FLAGS) -D__UNIX
.c.o: 
	$(CC) -c $<

# Definitions we'll need later (and that should rarely change)
HEADER_FILES = $(NAME).h md5.h sha1.h hashTable.h algorithms.h
SRC =  main.c match.c files.c hashTable.c helpers.c dig.c md5.c sha1.c hash.c cycles.c
OBJ =  main.o match.o helpers.o dig.o cycles.o
DOCS = Makefile README $(MD5GOAL).1 $(SHA1GOAL).1 CHANGES 
WINDOC = README.txt CHANGES.txt

#---------------------------------------------------------------------

all: linux

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



goals: md5deep sha1deep

linux: CC += -D__LINUX -DLARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
linux: goals

sunos: solaris
solaris: CC += -D__SOLARIS -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
solaris: goals

mac: CC += -D__MACOSX
mac: goals

macinstall: mac
	install -m 755 $(MD5GOAL) /usr/bin/
	install -m 755 $(SHA1GOAL) /usr/bin/
	install -m 755 $(MD5GOAL).1 /usr/share/man/man1/
	install -m 755 $(SHA1GOAL).1 /usr/share/man/man1/

unix: goals

# Cross compiling from Linux to Windows. See README for more info
cross: CC=$(CROSSCC) $(CROSSOPT)
cross: LINK_OPT = $(CROSSLINK)
cross: MD5GOAL  = $(CROSSMD5GOAL)
cross: SHA1GOAL = $(CROSSSHA1GOAL)
cross: goals
	strip *.exe

# See the README for information on Windows compilation
windows: CC = $(WINCC)
windows: LINK_OPT = $(CROSSLINK)
windows: MD5GOAL  = $(CROSSMD5GOAL)
windows: SHA1GOAL = $(CROSSSHA1GOAL)
windows: goals
	strip *.exe

#---------------------------------------------------------------------

install: goals
	install -m 755 $(MD5GOAL) $(BIN)/
	install -m 755 $(SHA1GOAL) $(BIN)/
	install -m 644 $(MD5GOAL).1 $(MAN)/
	install -m 644 $(SHA1GOAL).1 $(MAN)/

uninstall:
	rm -f -- $(BIN)/$(MD5GOAL) $(BIN)/$(SHA1GOAL) 
	rm -f -- $(MAN)/$(MD5GOAL).1 $(MAN)/$(SHA1GOAL).1

#---------------------------------------------------------------------


# This is used for debugging
preflight:
	grep -n RBF *.h *.c README CHANGES

nice:
	rm -f -- *~

clean: nice
	rm -f -- *.o
	rm -f -- $(MD5GOAL) $(SHA1GOAL) core *.core 
	rm -f -- $(CROSSMD5GOAL) $(CROSSSHA1GOAL) $(WINDOC)
	rm -f -- $(TAR_FILE).gz $(DEST_DIR).zip $(DEST_DIR).zip.gpg

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

win-doc:
	man ./$(MD5GOAL).1 | col -bx > README.txt
	cp CHANGES CHANGES.txt
	unix2dos CHANGES.txt
	unix2dos README.txt

cross-pkg: clean cross win-doc
	rm -f $(DEST_DIR).zip
	zip $(DEST_DIR).zip $(MD5GOAL).exe $(SHA1GOAL).exe $(WINDOC)
	rm -f $(WINDOC)

world: package cross-pkg
