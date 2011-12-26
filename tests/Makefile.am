EXTRA_DIST=README.txt tests.sh
TESTS=tests.sh
CLEANFILES=foo cow moo bar known1 known2 \
	hashlist-md5deep-full.txt    hashlist-hashdeep-full.txt \
	hashlist-md5deep-partial.txt hashlist-hashdeep-partial.txt

executable:
	svn propset svn:executable on *.sh

testclean:
	/bin/rm -rf /tmp/test/ /tmp/*.out /tmp/*.err *.out *.err $(CLEANFILES) tst ref
