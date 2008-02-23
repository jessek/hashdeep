
CC = gcc
CC_OPTS = -Wall -O2
LINK_OPTS = -lm
GOAL = md5deep

# This is the cross compiler for compiling Windows programs on Linux
# If you don't want to cross compile, don't worry about this
CROSSCC = /usr/local/cross-tools/i386-mingw32msvc/bin/gcc
CROSSOPT = -Wall -O2 -D__WIN32
CROSSLINK = -liberty
CROSSGOAL = $(GOAL).exe

# You shouldn't need to change anything below this line
#---------------------------------------------------------------------

# This is used when generating packages
VERSION = 0.16

# Where we get installed
BIN = /usr/local/bin
MAN = /usr/local/man/man1

# This should be commented out when debugging is done
#CC_OPTS += -D__DEBUG -ggdb

# Generic "how to compile C files"
CC += $(CC_OPTS)
.c.o: 
	$(CC) -c $<

# Definitions we'll need later (and that should never change)
HEADER_FILES = $(GOAL).h hashTable.h
SRC =  $(GOAL).c md5.c match.c files.c hashTable.c helpers.c
OBJ =  $(GOAL).o md5.o match.o files.o hashTable.o helpers.o
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
	$(CC) -D__UNIX -o $(GOAL) $(SRC) -lm

mac: CC += -D__MACOSX
mac: $(GOAL)

# Cross compiling from Linux to Windows. See README for more info
cross-win: CROSSCC += $(CROSSOPT) 
cross-win: $(SRC) $(HEADER_FILES)
	$(CROSSCC) -o $(CROSSGOAL) $(SRC) $(CROSSLINK)
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
	install -CDm 755 $(GOAL) $(BIN)/$(GOAL)
	install -CDm 644 $(GOAL).1 $(MAN)/$(GOAL).1

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

win-package: cross-win
	man ./$(GOAL).1 | col -bx > README.txt
	unix2dos README.txt
	zip md5deep-$(VERSION).zip $(GOAL).exe README.txt


cross-pkg: cross-win
	man ./$(GOAL).1 | col -b > README.txt
	cp CHANGES CHANGES.txt
	unix2dos CHANGES.txt
	unix2dos README.txt
	zip $(GOAL)-$(VERSION).zip $(GOAL).exe $(WINDOC)
	gpg -c $(GOAL)-$(VERSION).zip 

publish: clean cross-win package cross-pkg
	mv $(DEST_DIR).* ~/rnd/$(GOAL)/
	make clean


