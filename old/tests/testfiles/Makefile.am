#
# These test files and directories are used for testing the windows and 
# Unix versions. 
# The symlinktest contains a forrest of symlinks and only works on Unix.
# The zip file contains all of the files (including the tar of the symlinktest).
# Note that dot_file_⦿.txt has a Unicode character in its name.
#

FILES = 1072-at.txt \
	1072-plus.txt\
	copying.txt \
	deadbeef.txt \
	unicode_circled_bullet_⦿.txt \
	unicode_snowman_☃.txt \
	foo.txt \
	install.txt \
	one-byte-file.txt\
	zero-byte-file.txt\
	dir_a/web2 \
	dir_a/dir_b/web2a \
	bsd-hashes.txt \
	nsrlfile.txt

EXTRA_DIST = symlinktest.tar.gz testfiles.zip

symlinktest.tar.gz: 
	tar -c -f symlinktest.tar.gz -v -z --exclude "*.svn" symlinktest

testfiles.zip: $(FILES)
	zip testfiles.zip $(FILES)

# You'll need to get the symlink files on your own
