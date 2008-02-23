
CC = gcc
CC_OPTS = -Wall -O2
LINK_OPTS = -lm
GOAL = md5deep

# You shouldn't need to change anything below this line
#---------------------------------------------------------------------

# Where we get installed
BIN = /usr/local/bin
MAN = /usr/local/man/man1

# This should be commented out when debugging is done
#CC_OPTS += -D__DEBUG -ggdb

# This is used when generating packages
VERSION = 0.15

# Generic "how to compile C files"
CC += $(CC_OPTS)
.c.o: 
	$(CC) -c $<

# Definitions we'll need later (and that should never change)
HEADER_FILES = $(GOAL).h hashTable.h
SRC =  $(GOAL).c md5.c match.c files.c hashTable.c
DOCS = Makefile README $(GOAL).1 CHANGES TODO


all: linux
both: linux cross-win


linux: $(GOAL)
$(GOAL): $(SRC) $(HEADER_FILES)
	$(CC) -D__LINUX -o $(GOAL) $(SRC) $(LINK_OPTS)

unix: $(SRC) $(HEADER_FILES)
	$(CC) -D__UNIX -o $(GOAL) $(SRC) $(LINK_OPTS)

prof: $(SRC) $(HEADER_FILES)
	$(CC) -D__LINUX -pg -o $(GOAL) $(SRC) $(LINK_OPTS)

install: $(GOAL)
	install -CDm 755 $(GOAL) $(BIN)/$(GOAL)
	install -CDm 644 $(GOAL).1 $(MAN)/$(GOAL).1

uninstall:
	rm -f $(BIN)/$(GOAL) $(MAN)/$(GOAL).1


# See the README for information on Windows compilation
windows: clean $(GOAL).exe 
$(GOAL).exe: $(SRC) $(HEADER_FILES) 
	$(CC) -D__WIN32 $(SRC) -o $(GOAL).exe -liberty 


# This is used by the developer for cross-compiling to Windows
# See the README for more information
CROSSCC = /usr/local/cross-tools/i386-mingw32msvc/bin/gcc $(CC_OPTS)
cross-win: $(SRC) $(HEADER_FILES)
	$(CROSSCC) -D__WIN32 -o $(GOAL).exe $(SRC) -liberty

WINDOC = README.txt CHANGES.txt

cross-pkg: cross-win
	man ./$(GOAL).1 | col -b > README.txt
	cp CHANGES CHANGES.txt
	unix2dos CHANGES.txt
	unix2dos README.txt
	zip $(GOAL)-$(VERSION).zip $(GOAL).exe $(WINDOC)
	gpg -c $(GOAL)-$(VERSION).zip 


world: clean linux cross-win package cross-pkg


# This is used for debugging
preflight:
	grep -n RBF *.h *.c README

nice:
	rm -f *~

clean: nice
	rm -f $(OBJS) $(GOAL) core *.core $(GOAL).exe $(WINDOC)
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

publish: world
	mv $(DEST_DIR).* ~/rnd/$(GOAL)/
	make clean

