
CC = gcc
CC_OPTS = -Wall -O2
LINK_OPTS = -lm
GOAL = md5deep

# You can cross compile this program for Win32 using Linux and the 
# MinGW compiler. See the README for details. If you have already
# installed MinGW, put the location ($PREFIX) here:
CROSSBASE = /home/jessek/bin/cross-tools

# You shouldn't need to change anything below this line
#---------------------------------------------------------------------

# This is used when generating packages
VERSION = 1.0

# This should be commented out when debugging is done
#CC_OPTS += -D__DEBUG -ggdb

# Where we get installed
BIN = /usr/local/bin
MAN = /usr/local/man/man1

CROSSCC = $(CROSSBASE)/i386-mingw32msvc/bin/gcc
CROSSOPT = $(CC_OPTS) -D__WIN32
CROSSLINK = -liberty
CROSSGOAL = $(GOAL).exe

# Generic "how to compile C files"
CC += $(CC_OPTS)
.c.o: 
	$(CC) -c $<

# Definitions we'll need later (and that should never change)
HEADER_FILES = $(GOAL).h hashTable.h
SRC =  main.c md5.c match.c files.c hashTable.c helpers.c hash.c dig.c
OBJ =  main.o md5.o match.o files.o hashTable.o helpers.o hash.o dig.o
DOCS = Makefile README $(GOAL).1 CHANGES TODO
WINDOC = README.txt CHANGES.txt

#---------------------------------------------------------------------

all: linux

$(GOAL): $(SRC) $(HEADER_FILES) $(OBJ)
	$(CC) -o $(GOAL) $(OBJ) $(LINK_OPTS)

linux: CC += -D__LINUX -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
linux: $(GOAL)

sunos: solaris
solaris: CC += -D__SOLARIS -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
solaris: $(GOAL) 

unix: 
	$(CC) -D__UNIX -o $(GOAL) $(SRC) $(LINK_OPTS)

mac: CC += -D__MACOSX
mac: $(GOAL)

macinstall: $(GOAL)
	install -m 755 $(GOAL) /usr/bin/
	install -m 755 $(GOAL).1 /usr/share/man/man1/

# Cross compiling from Linux to Windows. See README for more info
cross: $(SRC) $(HEADER_FILES)
	$(CROSSCC) $(CROSSOPT) -o $(CROSSGOAL) $(SRC) $(CROSSLINK)
	strip $(CROSSGOAL)

# See the README for information on Windows compilation
windows: $(GOAL).exe 
$(GOAL).exe: $(SRC) $(HEADER_FILES) 
	$(CC) -D__WIN32 $(SRC) -o $(GOAL).exe -liberty 

# Sometimes I profile the code to find inefficient parts
prof: CC += -pg 
prof: linux

#---------------------------------------------------------------------

install: $(GOAL)
	install -m 755 $(GOAL) $(BIN)/$(GOAL)
	install -m 644 $(GOAL).1 $(MAN)/$(GOAL).1

uninstall:
	rm -f $(BIN)/$(GOAL) $(MAN)/$(GOAL).1

#---------------------------------------------------------------------


# This is used for debugging
preflight:
	grep -n RBF *.h *.c README

prep:
	rm -f $(OBJ)

nice:
	rm -f *~

clean: nice prep
	rm -f $(GOAL) core *.core $(GOAL).exe $(WINDOC)
	rm -f $(TAR_FILE).gz $(DEST_DIR).zip $(DEST_DIR).zip.gpg

#-------------------------------------------------------------------------

EXTRA_FILES = 
DEST_DIR = $(GOAL)-$(VERSION)
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

win-doc:
	man ./$(GOAL).1 | col -bx > README.txt
	cp CHANGES CHANGES.txt
	unix2dos CHANGES.txt
	unix2dos README.txt

win-package: cross win-doc
	rm -f $(GOAL)-$(VERSION).zip
	zip $(GOAL)-$(VERSION).zip $(GOAL).exe $(WINDOC)
	rm -f $(WINDOC)

cross-pkg: win-package
	gpg -c $(GOAL)-$(VERSION).zip 

publish: package win-package
	cp $(GOAL)-$(VERSION).* ~/xdrive/public_html/md5deep/
