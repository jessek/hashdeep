EXTRA_DIST=README.txt tests.sh
TESTS=tests.sh
CLEANFILES=

executable:
	svn propset svn:executable on *.sh

testclean:
	/bin/rm -rf /tmp/test/ /tmp/*.out /tmp/*.err *.out *.err foo cow moo bar known1 known2
