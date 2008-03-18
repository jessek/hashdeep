# This Makefile is separate from the autotools files in the
# top level directory. It is used only to clean up and
# publish files for the public web site.

clean:
	rm -f *~

manpages:
	rman

publish:
	scp index.html style.css jessekornblum@

all:
	scp *.html 