#!/bin/bash
# TEST_DIR is where the executable is that we are testing:
# GOOD_DIR is the directory containing the executable that is known to be good
# TESTFILES_DIR is where the test files are located.
# TMP is where the testfiles are installed for testing.
# HTMP is the location of the testfiles to be passed to the hashing programs
#  this matters on windows, where bash (and the Unix utilities) will have a 
#  different set of paths than cygwin-compiled executables.

# to deal with newline issues, we just always remove \r from the output


TESTFILES_DIR=testfiles

unicode="yes"
verbose="no"

# Process args
# http://www.zanecorp.com/wiki/index.php/Processing_Options_in_Bash
prog=$0
until [ x$1 == x ]; do
    case $1 in
        -n|--nounicode)
            unicode="no";;
	-v|--verbose)
	    verbose="yes";;
	-h|--help|*)
	    echo "usage: $prog [options]"
	    echo " -h, --help      -- print this message"
	    echo " -n, --nounicode -- delete any test file that begin "
            echo "                    with \"unicode\*\" in its file name."
	    echo " -v, --verbose   -- print commands when run"
	    exit 0
    esac
    shift # move the arg list to the next option or '--'
done
shift # remove the '--', now $1 positioned at first argument if any

case `uname -s` in 
  CYGWIN*WOW64)
    echo You are running on 64-bit Windows
    EXE="64.exe"
    TMP='/cygdrive/c/tmp'
    HTMP='c:/tmp'
    windows=1
    ;;
  CYGWIN*)
    echo You are running on 32-bit Windows
    EXE=".exe"
    TMP='/cygdrive/c/tmp'
    WTMP='c:/tmp'
    windows=1
    ;;
  *)
    echo You are running on Linux or Mac.
    EXE=""
    TMP='/tmp'
    HTMP='/tmp'
    windows=""
    ;;
esac

# TEST_DIR is the program under test
if [ x$TEST_DIR = x ] ; then TEST_DIR=../src; fi
if [ ! -r $TEST_DIR/md5deep$EXE ] ; then
  echo cannot find md5deep$EXE in $TEST_DIR
  echo Please set TEST_DIR and try again
  exit 1
fi

# GOOD_DIR is the program that is known to be good
if [ x$GOOD_DIR = x ] ; then GOOD_DIR=$1 ; fi
if [ x$GOOD_DIR = x ] ; then
  WHERE=`which md5deep`
  GOOD_DIR=`dirname $WHERE`
fi
if [ x$GOOD_DIR = x ] ; then
  echo Unable to locate installed md5deep executable.
  echo Define GOOD_DIR or specify the directory with the known good executables
  echo on the command line.
  exit 1
fi


if [ ! -d $TMP ]; then
  echo No $TMP directory detected. Will try to create.
  mkdir $TMP
  if [ ! -d $TMP ]; then
    echo Cannot create $TMP.
    echo Finished.
    exit 1
  fi
fi

echo Reference output will go to ref/
echo Test output will go to tst/
echo Files to test will go in $TMP/test
echo 
/bin/rm -rf ref tst
mkdir ref tst

echo Removing files in $TMP/test
/bin/rm -rf $TMP/test/

echo Installing test files in $TMP/test
mkdir $TMP/test
(cd $TESTFILES_DIR >/dev/null; tar cf - .) | (cd $TMP/test;tar xpBf - )


echo Removing any '.svn' files that might have gotten into $TMP/test by accident
find $TMP/test -depth -name '.svn' -exec rm -rf {} \;


if [ $unicode = "no" ] ; then
  echo Removing filenames beginning with unicode
  find $TMP/test -name 'unicode*.txt' -exec rm {} \;
fi


echo Erasing the hashlist database files in the current directory
/bin/rm -f hashlist-*.txt

echo Creating hashlist files with no-match-em and two files with installed program
$GOOD_DIR/hashdeep$EXE -l $HTMP/test/deadbeef.txt  $HTMP/test/foo.txt > hashlist-hashdeep-partial.txt
tail -1 hashlist-hashdeep-partial.txt | sed s+$HTMP/test/foo.txt+/no/match/em+ | sed s/[012345]/6/g >> hashlist-hashdeep-partial.txt

$GOOD_DIR/md5deep$EXE -l $HTMP/test/deadbeef.txt  $HTMP/test/foo.txt > hashlist-md5deep-partial.txt
tail -1 hashlist-md5deep-partial.txt | sed s+$HTMP/test/foo.txt+/no/match/em+ | sed s/[012345]/6/g >> hashlist-md5deep-partial.txt

$GOOD_DIR/hashdeep$EXE -l -r $HTMP/test > hashlist-hashdeep-full.txt 2>/dev/null
$GOOD_DIR/md5deep$EXE -l -r $HTMP/test > hashlist-md5deep-full.txt   2>/dev/null

# Now run the tests!

for mode in generate test
do
  echo $mode mode.
  if [ $mode = "generate" ] 
  then
    # generate mode uses the known good code
    BASE=$GOOD_DIR
    S=""			# nothing extra
  else
    # test mode uses the version we are developing
    BASE=$TEST_DIR
    S=""			# nothing extra
  fi
  fails=0
  for ((i=1;;i++))
  do 
   cmd=""
   case $i in
   # try lots of different versions of md5deep

     1) cmd="$BASE/md5deep$EXE $S -r    $HTMP/test                       " ;;
     2) cmd="$BASE/md5deep$EXE $S -p512 $HTMP/test/deadbeef.txt          " ;;
     3) cmd="$BASE/md5deep$EXE $S -zr   $HTMP/test                       " ;;
     4) cmd="$BASE/md5deep$EXE $S -b    $HTMP/test/deadbeef.txt          " ;;
     5) cmd="$BASE/md5deep$EXE $S -m hashlist-md5deep-partial.txt -r $HTMP/test  " ;;
     6) cmd="$BASE/md5deep$EXE $S -x hashlist-md5deep-partial.txt -r $HTMP/test  " ;;
     7) cmd="$BASE/md5deep$EXE $S -M hashlist-md5deep-partial.txt -r $HTMP/test  " ;;
     8) cmd="$BASE/md5deep$EXE $S -X hashlist-md5deep-partial.txt -r $HTMP/test  " ;;
     9) cmd="$BASE/md5deep$EXE $S -m hashlist-md5deep-partial.txt -w -r $HTMP/test  " ;;
    10) cmd="$BASE/md5deep$EXE $S -m hashlist-md5deep-partial.txt -n -r $HTMP/test  " ;;
    11) cmd="$BASE/md5deep$EXE $S -w -a deadbeefb303ba89ae055ad0234eb7e8  -r $HTMP/test  " ;;
    12) cmd="$BASE/md5deep$EXE $S -w -A deadbeefb303ba89ae055ad0234eb7e8  -r $HTMP/test  " ;;
    13) cmd="$BASE/md5deep$EXE $S -k    $HTMP/test/deadbeef.txt         " ;;
    14) cmd="$BASE/md5deep$EXE $S -i1000 -r $HTMP/test                  " ;;
    15) cmd="$BASE/md5deep$EXE $S -I1000 -r $HTMP/test                  " ;;
    16) cmd="$BASE/md5deep$EXE $S  -p512 -r $HTMP/test                  " ;;
    17) cmd="$BASE/md5deep$EXE $S -Z $HTMP/test/*.txt "      ;; 
    18) cmd="$BASE/md5deep$EXE $S -m  $TESTFILES_DIR/known.txt .svn" ;;	  
    19) cmd="$BASE/md5deep$EXE $S -Sm $TESTFILES_DIR/known.txt .svn" ;;	  
    20) cmd="$BASE/md5deep$EXE $S -sm $TESTFILES_DIR/known.txt .svn" ;;
    21) cmd="$BASE/md5deep$EXE    /dev/null ";;

    # Now try all of the different deeps in regular mode and piecewise hashing mode
    22) cmd="$BASE/md5deep$EXE $S -r $HTMP/test            " ;;
    23) cmd="$BASE/md5deep$EXE $S -p512 -r $HTMP/test	" ;;

    24) cmd="$BASE/sha1deep$EXE $S -r $HTMP/test	" ;;
    25) cmd="$BASE/sha1deep$EXE $S -p512 -r $HTMP/test  " ;;

    26) cmd="$BASE/sha256deep$EXE $S -r $HTMP/test	" ;;
    27) cmd="$BASE/sha256deep$EXE $S -p512 -r $HTMP/test" ;;

    28) cmd="$BASE/tigerdeep$EXE $S -r $HTMP/test	" ;;
    29) cmd="$BASE/tigerdeep$EXE $S -p512 -r $HTMP/test " ;;

    30) cmd="$BASE/whirlpooldeep$EXE $S -r $HTMP/test	" ;;
    31) cmd="$BASE/whirlpooldeep$EXE $S -p512 -r $HTMP/test " ;;

    32) cmd="$BASE/hashdeep$EXE $S -r   $HTMP/test	" ;;
    33) cmd="$BASE/hashdeep$EXE $S -p512 -r $HTMP/test	" ;;

    34) cmd="$BASE/hashdeep$EXE $S -m -k hashlist-hashdeep-partial.txt $HTMP/test/*.txt  " ;;
    35) cmd="$BASE/hashdeep$EXE $S -M -k hashlist-hashdeep-partial.txt $HTMP/test/*.txt  " ;;
    36) cmd="$BASE/hashdeep$EXE $S -w -m -k hashlist-hashdeep-partial.txt $HTMP/test/*.txt  " ;;
    37) cmd="$BASE/hashdeep$EXE $S -x -k hashlist-hashdeep-partial.txt $HTMP/test/*.txt  " ;;
    38) cmd="$BASE/hashdeep$EXE $S -x -w -k hashlist-hashdeep-partial.txt $HTMP/test/*.txt  " ;;
    39) cmd="$BASE/hashdeep$EXE $S -r -a -k hashlist-hashdeep-full.txt $HTMP/test " ;;
    40) cmd="$BASE/hashdeep$EXE $S -v -r -a -k hashlist-hashdeep-full.txt $HTMP/test "  ;;
    41) cmd="$BASE/hashdeep$EXE   /dev/null		";;

     # Finally, the stdin tests

    42) cmd="echo README.txt | $BASE/hashdeep$EXE"      ;;
    43) cmd="echo README.txt | $BASE/md5deep$EXE"       ;;
    44) cmd="echo README.txt | $BASE/sha1deep$EXE"      ;;
    45) cmd="echo README.txt | $BASE/sha256deep$EXE"    ;;
    46) cmd="echo README.txt | $BASE/whirlpooldeep$EXE" ;;
   esac
   if [ x"$cmd" = "x" ]
   then 
     break
   fi
   ###
   ### run the test!
   ### Standard output gets sorted to deal with multi-threading issues
   ### Standard error simply gets captured
   
   /bin/echo -n $mode $i ...
   /bin/rm -f $HTMP/test$i.out

   if [ $verbose = "yes" ]; then
     echo $cmd
   fi
   $cmd 2>test$i.err | sed s+$BASE/++ | sort  > test$i.out
   if [ $mode = "generate" ]; then
     tr -d \\r < test$i.out > ref/test$i.out
     tr -d \\r < test$i.err > ref/test$i.err
     echo ok
   else
     tr -d \\r < test$i.out > hold; mv hold test$i.out
     tr -d \\r < test$i.err > hold; mv hold test$i.err
   fi
   if [ $mode = "test" ]
   then
     fail=no
     mv -f test$i.out tst/test$i.out
     mv -f test$i.err tst/test$i.err
     if ! diff ref/test$i.out tst/test$i.out ; 
     then 
       echo TEST $i FAILED --- STDOUT DIFFERENT
       echo COMMAND: $cmd
       fail=yes
     fi
     if ! diff ref/test$i.err tst/test$i.err ; 
     then 
       echo TEST $i FAILED --- STDERR DIFFERENT
       echo COMMAND: $cmd
       fail=yes
     fi
     if [ $fail = "yes" ]
     then
       ((fails++))
     else
       echo passes.
     fi
   fi
  done
done
echo Total Failures: $fails
exit $fails
  
  
