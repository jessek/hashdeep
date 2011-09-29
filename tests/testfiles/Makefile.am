#
# These test files and directories are used for testing the windows and 
# Unix versions. 
# The symlinktest contains a forrest of symlinks and only works on Unix.
# The zip file contains all of the files (including the tar of the symlinktest).
# Note that dot_file_⦿.txt has a Unicode character in its name.
#

FILES := 1072-at.txt \
	1072-plus.txt\
	copying.txt \
	deadbeef.txt \
	dot_file_⦿.txt \
	foo.txt \
	install.txt \
	one-byte-file\
	zero-byte-file\
	dir_a/web2 \
	dir_a/dir_b/web2a \
	dir_c/Declaration_Engrav_Pg1of1_AC.jpg \
	dir_c/Declaration_Pg1of1_AC.jpg 

EXTRA_DIST = $(FILES) symlinktest.tar.gz testfiles.zip

symlinktest.tar.gz: 
	tar -c -f symlinktest.tar.gz -v -z --exclude "*.svn" symlinktest

testfiles.zip: $(EXTRA_DIST)
	zip testfiles.zip $(EXTRA_DIST)

# You'll need to get the symlink files on your own