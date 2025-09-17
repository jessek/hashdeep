#!/bin/bash
# TEST_BIN is where the executable is that we are testing:
# GOOD_BIN is the directory containing the executable that is known to be good
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

# TMP is the temp directory to use in this script
# HTMP is the temp dierctory to pass to the executable.

case `uname -s` in 
  CYGWIN*WOW64)
    echo You are running on 64-bit Windows
    EXE="64.exe"
    TMP='/cygdrive/c/tmp/test'
    HTMP='c:/tmp/test'
    windows=1
    ;;
  CYGWIN*)
    echo You are running on 32-bit Windows
    EXE=".exe"
    TMP='/cygdrive/c/tmp/test'
    HTMP='c:/tmp/test'
    windows=1
    ;;
  *)
    echo You are running on Linux or Mac.
    EXE=""
    TMP='/tmp/test'
    HTMP='/tmp/test'
    windows=""
    ;;
esac

# TEST_BIN is the program under test
if [ x$TEST_BIN = x ] ; then TEST_BIN=../src; fi
if [ ! -r $TEST_BIN/md5deep$EXE ] ; then
  echo cannot find md5deep$EXE in $TEST_BIN
  echo Please set TEST_BIN with the directory of the executables being tested.
  exit 1
fi

# GOOD_BIN is the program that is known to be good
if [ x$GOOD_BIN = x ] ; then GOOD_BIN=$1 ; fi
if [ x$GOOD_BIN = x ] ; then
  WHERE=`which md5deep 2>/dev/null`
  GOOD_BIN=`dirname $WHERE`
fi
if [ x$GOOD_BIN = x ] ; then
  echo Unable to locate installed md5deep executable.
  echo Define GOOD_BIN or specify the directory with the known good executables
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

good_ver=`$GOOD_BIN/md5deep -v`
test_ver=`$TEST_BIN/md5deep -v`
echo REFERENCE VERSION: $good_ver
echo TEST VERSION:      $test_ver
if [ $good_ver = $test_ver ]; then
  echo It makes no sense to test a version against itself.
  echo Press return to continue test, ^c to terminate.
  read
fi
echo Reference output will go to ref/
echo Test output will go to tst/
echo Files to test will go in $TMP
echo 
/bin/rm -rf ref tst
mkdir ref tst

echo Removing files in $TMP
/bin/rm -rf $TMP

echo Installing test files in $TMP
mkdir $TMP
(cd $TESTFILES_DIR >/dev/null; tar cf - .) | (cd $TMP;tar xpBf - )


echo Removing any '.svn' files that might have gotten into $TMP by accident
find $TMP -depth -name '.svn' -exec rm -rf {} \;


if [ $unicode = "no" ] ; then
  echo Removing filenames beginning with unicode
  find $TMP -name 'unicode*.txt' -exec rm {} \;
fi


echo Erasing the hashlist database files in the current directory
/bin/rm -f hashlist-*.txt

echo Creating hashlist files with no-match-em and two files with installed program
$GOOD_BIN/hashdeep$EXE -l $HTMP/deadbeef.txt  $HTMP/foo.txt > hashlist-hashdeep-partial.txt
tail -1 hashlist-hashdeep-partial.txt | sed s+$HTMP/foo.txt+/no/match/em+ \
    | sed s/[012345]/6/g >> hashlist-hashdeep-partial.txt

$GOOD_BIN/md5deep$EXE -l $HTMP/deadbeef.txt  $HTMP/foo.txt > hashlist-md5deep-partial.txt
tail -1 hashlist-md5deep-partial.txt | sed s+$HTMP/foo.txt+/no/match/em+ \
    | sed s/[012345]/6/g >> hashlist-md5deep-partial.txt

$GOOD_BIN/hashdeep$EXE -l -r $HTMP > hashlist-hashdeep-full.txt 2>/dev/null
$GOOD_BIN/md5deep$EXE  -l -r $HTMP > hashlist-md5deep-full.txt  2>/dev/null

# Create the test files for audit test case
/bin/rm -f foo bar moo cow known1 known2
echo foo > foo
echo bar > bar
echo moo > moo
echo cow > cow
$GOOD_BIN/hashdeep$EXE -bc md5 foo bar  > known1
$GOOD_BIN/hashdeep$EXE -bc sha1 moo cow > known2

# Now run the tests!

for mode in generate test
do
  echo $mode mode.
  if [ $mode = "generate" ] 
  then
    # generate mode uses the known good code
    BASE=$GOOD_BIN
  else
    # test mode uses the version we are developing
    BASE=$TEST_BIN
  fi
  fails=0
  for ((i=1;;i++))
  do 
   cmd=""
   case $i in
   # try lots of different versions of md5deep

     1) cmd="$BASE/md5deep$EXE -r    $HTMP                       " ;;
     2) cmd="$BASE/md5deep$EXE -p512 $HTMP/deadbeef.txt          " ;;
     3) cmd="$BASE/md5deep$EXE -zr   $HTMP                       " ;;
     4) cmd="$BASE/md5deep$EXE -b    $HTMP/deadbeef.txt          " ;;
     5) cmd="$BASE/md5deep$EXE -m hashlist-md5deep-partial.txt -r $HTMP  " ;;
     6) cmd="$BASE/md5deep$EXE -x hashlist-md5deep-partial.txt -r $HTMP  " ;;
     7) cmd="$BASE/md5deep$EXE -M hashlist-md5deep-partial.txt -r $HTMP  " ;;
     8) cmd="$BASE/md5deep$EXE -X hashlist-md5deep-partial.txt -r $HTMP  " ;;
     9) cmd="$BASE/md5deep$EXE -m hashlist-md5deep-partial.txt -w -r $HTMP  " ;;
    10) cmd="$BASE/md5deep$EXE -m hashlist-md5deep-partial.txt -n -r $HTMP  " ;;
    11) cmd="$BASE/md5deep$EXE -w -a deadbeefb303ba89ae055ad0234eb7e8  -r $HTMP  " ;;
    12) cmd="$BASE/md5deep$EXE -w -A deadbeefb303ba89ae055ad0234eb7e8  -r $HTMP  " ;;
    13) cmd="$BASE/md5deep$EXE -k    $HTMP/deadbeef.txt         " ;;
    14) cmd="$BASE/md5deep$EXE -i1000 -r $HTMP                  " ;;
    15) cmd="$BASE/md5deep$EXE -I1000 -r $HTMP                  " ;;
    16) cmd="$BASE/md5deep$EXE -p512 -r $HTMP                  " ;;
    17) cmd="$BASE/md5deep$EXE -Z $HTMP/*.txt "      ;; 
    18) cmd="$BASE/md5deep$EXE -m  $TESTFILES_DIR/known.txt $HTMP" ;;	  
    19) cmd="$BASE/md5deep$EXE -Sm $TESTFILES_DIR/known.txt $HTMP" ;;	  
    20) cmd="$BASE/md5deep$EXE -sm $TESTFILES_DIR/known.txt $HTMP" ;;
    21) cmd="$BASE/md5deep$EXE /dev/null ";;

    # Now try all of the different deeps in regular mode and piecewise hashing mode
    22) cmd="$BASE/md5deep$EXE -r $HTMP            " ;;
    23) cmd="$BASE/md5deep$EXE -p512 -r $HTMP	" ;;

    24) cmd="$BASE/sha1deep$EXE -r $HTMP	" ;;
    25) cmd="$BASE/sha1deep$EXE -p512 -r $HTMP  " ;;

    26) cmd="$BASE/sha256deep$EXE -r $HTMP	" ;;
    27) cmd="$BASE/sha256deep$EXE -p512 -r $HTMP" ;;

    28) cmd="$BASE/tigerdeep$EXE -r $HTMP	" ;;
    29) cmd="$BASE/tigerdeep$EXE -p512 -r $HTMP " ;;

    30) cmd="$BASE/whirlpooldeep$EXE -r $HTMP	" ;;
    31) cmd="$BASE/whirlpooldeep$EXE -p512 -r $HTMP " ;;

    32) cmd="$BASE/hashdeep$EXE -r   $HTMP	" ;;
    33) cmd="$BASE/hashdeep$EXE -p512 -r $HTMP	" ;;

    34) cmd="$BASE/hashdeep$EXE -m -k hashlist-hashdeep-partial.txt $HTMP/*.txt  " ;;
    35) cmd="$BASE/hashdeep$EXE -M -k hashlist-hashdeep-partial.txt $HTMP/*.txt  " ;;
    36) cmd="$BASE/hashdeep$EXE -w -m -k hashlist-hashdeep-partial.txt $HTMP/*.txt  " ;;
    37) cmd="$BASE/hashdeep$EXE -x -k hashlist-hashdeep-partial.txt $HTMP/*.txt  " ;;
    38) cmd="$BASE/hashdeep$EXE -x -w -k hashlist-hashdeep-partial.txt $HTMP/*.txt  " ;;
    39) cmd="$BASE/hashdeep$EXE -r -a -k hashlist-hashdeep-full.txt $HTMP " ;;
    40) cmd="$BASE/hashdeep$EXE -v -r -a -k hashlist-hashdeep-full.txt $HTMP "  ;;
    41) cmd="$BASE/hashdeep$EXE   /dev/null		";;

     # The stdin tests

    42) cmd="echo README.txt | $BASE/hashdeep$EXE"      ;;
    43) cmd="echo README.txt | $BASE/md5deep$EXE"       ;;
    44) cmd="echo README.txt | $BASE/sha1deep$EXE"      ;;
    45) cmd="echo README.txt | $BASE/sha256deep$EXE"    ;;
    46) cmd="echo README.txt | $BASE/whirlpooldeep$EXE" ;;

     # Additional tests as errors are discovered
    47) cmd="$BASE/hashdeep$EXE -vvvbak known1 -k known2 foo bar moo cow"      ;;

     # BSD style hashes, iLook hashes.
     # iLook has different behavior with the algorithms, so we test with all
    48) cmd="$BASE/md5deep$EXE -Sm $TESTFILES_DIR/bsd-hashes.txt -r $HTMP" ;;
    49) cmd="$BASE/md5deep$EXE -m $TESTFILES_DIR/ilookv4.hsh -r $HTMP" ;;
    50) cmd="$BASE/sha1deep$EXE -m $TESTFILES_DIR/ilookv4.hsh -r $HTMP" ;;
    51) cmd="$BASE/md5deep$EXE -m $TESTFILES_DIR/nsrlfile.txt  -r $HTMP" ;;
    52) cmd="$BASE/sha1deep$EXE -m $TESTFILES_DIR/nsrlfile.txt -r $HTMP" ;;
       

   esac
   if [ x"$cmd" = "x" ]
   then 
     break
   fi
   ###
   ### run the test!
   ### Standard output gets sorted to deal with multi-threading issues
   ### Standard error simply gets captured
   ##
   
   /bin/echo -n $mode $i ...
   /bin/rm -f $HTMP$i.out

   if [ $verbose = "yes" ]; then
     echo $cmd
   fi

   # STDOUT needs to have the executable directory removed.
   # On Windows, path names are inconsistent between the name that is provided
   # and the name the command thinks that it is being run under.
   # So we unify the prompt and remove C:.*hashdeep and replace with hashdeep.

   ###
   ### HERE IT IS:
   ###

   $cmd 2>test$i.err | sed s+$BASE/++ \
        | sed s+"## C:[^ ]*>"+"## C:>"+ | sed s+"C:[^> ]*hashdeep"+C:/hashdeep+ | sort  > test$i.out
   ###
   ### 

   if [ $mode = "generate" ]; then
     tr -d \\r < test$i.out > ref/test$i.out
     tr -d \\r < test$i.err > ref/test$i.err

     # extra addition for test 47
     if [ $i = 47 ]; then
       if grep 'Input files examined' ref/test$i.out ; then
         echo Fixup for test 47 no longer required
       else
         echo Applying fixup for test 47
         echo "   Input files examined: 0" >> ref/test$i.out
         echo "  Known files expecting: 0" >> ref/test$i.out
         sort ref/test$i.out > ref/test$i.out2
         mv ref/test$i.out2 ref/test$i.out
       fi
     fi

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
     if ! diff ref/test$i.out tst/test$i.out ; then 
       echo ======================================
       echo TEST $i FAILED --- STDOUT DIFFERENT
       echo COMMAND: $cmd
       echo REFERENCE STDOUT:
       cat ref/test$i.out
       echo
       echo TEST STDOUT:
       cat tst/test$i.out
       echo ======================================
       fail=yes
     fi
     if ! diff ref/test$i.err tst/test$i.err ; then 
       echo ======================================
       echo TEST $i FAILED --- STDERR DIFFERENT
       echo COMMAND: $cmd
       echo REFERENCE STDERR:
       cat ref/test$i.err
       echo
       echo TEST STDERR:
       cat tst/test$i.err
       echo ======================================
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
  
  
