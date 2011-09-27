#!/bin/bash
# TEST_DIR is where the executable is that we are testing:
# GOOD_DIR is the directory containing the executable that is known to be good
# You can set this environment variable in your startup scripts.
# TESTFILES_DIR is where the test files are located. They are 
# installed in /tmp so that names will be consistent.

TEST_DIR=../src
TESTFILES_DIR=testfiles

if [ x$GOOD_DIR = x ] ; then GOOD_DIR=$1 ; fi
if [ x$GOOD_DIR = x ] ; then
  TMP=`which md5deep`
  GOOD_DIR=`dirname $TMP`
fi
if [ x$GOOD_DIR = x ] ; then
  echo Unable to locate installed md5deep executable.
  echo Define GOOD_DIR or specify the directory with the known good executables
  echo on the command line.
  exit 1
fi

echo Reference output will go to ref/
echo Test output will go to tst/
/bin/rm -rf ref tst
mkdir ref tst

echo Removing files in /tmp/test
/bin/rm -rf /tmp/test/

echo Installing test files in /tmp/test
mkdir /tmp/test
(cd $TESTFILES_DIR >/dev/null; tar cf - .) | (cd /tmp/test;tar xpBf - )

echo Removing any '.svn' files that might have gotten into /tmp/test by accident
find /tmp/test -depth -name '.svn' -exec rm -rf {} \;

echo Erasing the hashlist database files in the current directory
/bin/rm -f hashlist-*.txt

echo Creating hashlist files with no-match-em and two files with installed program
$GOOD_DIR/hashdeep -l /tmp/test/deadbeef.txt  /tmp/test/foo.txt > hashlist-hashdeep-partial.txt
tail -1 hashlist-hashdeep-partial.txt | sed s+/tmp/test/foo.txt+/no/match/em+ | sed s/[012345]/6/g >> hashlist-hashdeep-partial.txt

$GOOD_DIR/md5deep -l /tmp/test/deadbeef.txt  /tmp/test/foo.txt > hashlist-md5deep-partial.txt
tail -1 hashlist-md5deep-partial.txt | sed s+/tmp/test/foo.txt+/no/match/em+ | sed s/[012345]/6/g >> hashlist-md5deep-partial.txt

$GOOD_DIR/hashdeep -l -r /tmp/test > hashlist-hashdeep-full.txt
$GOOD_DIR/md5deep -l -r /tmp/test > hashlist-md5deep-full.txt


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

     1) cmd="$BASE/md5deep $S -r    /tmp/test                       " ;;
     2) cmd="$BASE/md5deep $S -p512 /tmp/test/deadbeef.txt          " ;;
     3) cmd="$BASE/md5deep $S -zr   /tmp/test                       " ;;
     4) cmd="$BASE/md5deep $S -b    /tmp/test/deadbeef.txt          " ;;
     5) cmd="$BASE/md5deep $S -m hashlist-md5deep-partial.txt -r /tmp/test  " ;;
     6) cmd="$BASE/md5deep $S -x hashlist-md5deep-partial.txt -r /tmp/test  " ;;
     7) cmd="$BASE/md5deep $S -M hashlist-md5deep-partial.txt -r /tmp/test  " ;;
     8) cmd="$BASE/md5deep $S -X hashlist-md5deep-partial.txt -r /tmp/test  " ;;
     9) cmd="$BASE/md5deep $S -m hashlist-md5deep-partial.txt -w -r /tmp/test  " ;;
    10) cmd="$BASE/md5deep $S -m hashlist-md5deep-partial.txt -n -r /tmp/test  " ;;
    11) cmd="$BASE/md5deep $S -w -a deadbeefb303ba89ae055ad0234eb7e8  -r /tmp/test  " ;;
    12) cmd="$BASE/md5deep $S -w -A deadbeefb303ba89ae055ad0234eb7e8  -r /tmp/test  " ;;
    13) cmd="$BASE/md5deep $S -k    /tmp/test/deadbeef.txt         " ;;
    14) cmd="$BASE/md5deep $S -i1000 -r /tmp/test                  " ;;
    15) cmd="$BASE/md5deep $S -I1000 -r /tmp/test                  " ;;
    16) cmd="$BASE/md5deep $S  -p512 -r /tmp/test                  " ;;
    17) cmd="$BASE/md5deep $S -Z /tmp/test/*.txt "      ;; 
    18) cmd="$BASE/md5deep $S -m  $TESTFILES_DIR/known.txt .svn" ;;	  
    19) cmd="$BASE/md5deep $S -Sm $TESTFILES_DIR/known.txt .svn" ;;	  
    20) cmd="$BASE/md5deep $S -sm $TESTFILES_DIR/known.txt .svn" ;;
    21) cmd="$BASE/md5deep    /dev/null ";;

    # Now try all of the different deeps in regular mode and piecewise hashing mode
    22) cmd="$BASE/md5deep $S -r /tmp/test                  " ;;
    23) cmd="$BASE/md5deep $S -p512 -r /tmp/test                  " ;;

    24) cmd="$BASE/sha1deep $S -r /tmp/test                  " ;;
    25) cmd="$BASE/sha1deep $S -p512 -r /tmp/test                  " ;;

    26) cmd="$BASE/sha256deep $S -r /tmp/test                  " ;;
    27) cmd="$BASE/sha256deep $S -p512 -r /tmp/test                  " ;;

    28) cmd="$BASE/tigerdeep $S -r /tmp/test                  " ;;
    29) cmd="$BASE/tigerdeep $S -p512 -r /tmp/test                  " ;;

    30) cmd="$BASE/whirlpooldeep $S -r /tmp/test                  " ;;
    31) cmd="$BASE/whirlpooldeep $S -p512 -r /tmp/test                  " ;;

    32) cmd="$BASE/hashdeep $S -r   /tmp/test                      " ;;
    33) cmd="$BASE/hashdeep $S -p512 -r /tmp/test                  " ;;

    34) cmd="$BASE/hashdeep $S -m -k hashlist-hashdeep-partial.txt /tmp/test/*.txt  " ;;
    35) cmd="$BASE/hashdeep $S -M -k hashlist-hashdeep-partial.txt /tmp/test/*.txt  " ;;
    36) cmd="$BASE/hashdeep $S -w -m -k hashlist-hashdeep-partial.txt /tmp/test/*.txt  " ;;
    37) cmd="$BASE/hashdeep $S -x -k hashlist-hashdeep-partial.txt /tmp/test/*.txt  " ;;
    38) cmd="$BASE/hashdeep $S -x -w -k hashlist-hashdeep-partial.txt /tmp/test/*.txt  " ;;
    39) cmd="$BASE/hashdeep $S -r -a -k hashlist-hashdeep-full.txt /tmp/test " ;;
    40) cmd="$BASE/hashdeep $S -v -r -a -k hashlist-hashdeep-full.txt /tmp/test "  ;;
    41) cmd="$BASE/hashdeep   /dev/null ";;

     # Finally, the stdin tests

    42) cmd="echo README.txt | $BASE/hashdeep"       ;;
    43) cmd="echo README.txt | $BASE/md5deep "      ;;
    44) cmd="echo README.txt | $BASE/sha1deep "      ;;
    45) cmd="echo README.txt | $BASE/sha256deep "    ;;
    46) cmd="echo README.txt | $BASE/whirlpooldeep " ;;
   esac
   if [ x"$cmd" = "x" ]
   then 
     break
   fi
   ###
   ### run the test!
   ### Standard output gets sorted to deal with multi-threading issues
   ### Standard error simply gets captured
   
   /bin/echo -n Test $i ...
   /bin/rm -f /tmp/test$i.out

   $cmd 2>/tmp/test$i.err | sed s+$BASE/++ | sort  > /tmp/test$i.out
   if [ $mode = "generate" ]
   then
     echo ""
     mv -f /tmp/test$i.out ref/test$i.out
     mv -f /tmp/test$i.err ref/test$i.err
   fi
   if [ $mode = "test" ]
   then
     fail=no
     mv -f /tmp/test$i.out tst/test$i.out
     mv -f /tmp/test$i.err tst/test$i.err
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
  
  
