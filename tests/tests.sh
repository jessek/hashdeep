#!/bin/sh
# TEST_DIR is where the executable is that we are testing:
TEST_DIR=../src
# GOOD_DIR is the executable that is known to be good
GOOD_DIR=/opt/local/bin

echo Installing test files in /tmp/test
/bin/rm -rf /tmp/test
mkdir /tmp/test
cp -p ../testfiles/* /tmp/test

echo Creating hashlist files with no-match-em and two files with installed program

if [ ! -r hashlist-hashdeep-partial.txt ] ;
then
  hashdeep -l /tmp/test/deadbeef.txt  /tmp/test/foo.txt > hashlist-hashdeep-partial.txt
  tail -1 hashlist-hashdeep-partial.txt | sed s+/tmp/test/foo.txt+/no/match/em+ | sed s/[012345]/6/g >> hashlist-hashdeep-partial.txt
fi

if [ ! -r hashlist-md5deep-partial.txt ] ;
then
  md5deep -l /tmp/test/deadbeef.txt  /tmp/test/foo.txt > hashlist-md5deep-partial.txt
  tail -1 hashlist-md5deep-partial.txt | sed s+/tmp/test/foo.txt+/no/match/em+ | sed s/[012345]/6/g >> hashlist-md5deep-partial.txt
fi

echo creating the full list from /tmp/test
if [ ! -r hashlist-hashdeep-full.txt ] ;
then
  hashdeep -l -r /tmp/test > hashlist-hashdeep-full.txt
fi

if [ ! -r hashlist-md5deep-full.txt ] ;
then
  md5deep -l -r /tmp/test > hashlist-md5dee-full.txt
fi

for mode in generate test
do
  echo $mode mode.
  if [ $mode = "generate" ] 
  then
    # generate mode uses the known good code
    BASE=$GOOD_DIR
    S=""			# no single threading code
  else
    # test mode uses the version we are developing
    BASE=$TEST_DIR
    S="-j0"			# Single threaded
  fi
  fails=0
  done=0
  for ((i=1;$done==0;i++))
  do 
   /bin/echo -n Test $i ...
   /bin/rm -f /tmp/test$i.out
   cmd=""
   case $i in
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
    13) cmd="$BASE/md5deep $S -k    /tmp/test/deadbeef.txt          " ;;
    14) cmd="$BASE/md5deep $S -i1000 -r /tmp/test                   " ;;
    15) cmd="$BASE/md5deep $S -I1000 -r /tmp/test                   " ;;
    16) cmd="$BASE/hashdeep $S -r   /tmp/test                       " ;;
    17) cmd="$BASE/hashdeep $S -p512 -r /tmp/test                     " ;;
    18) cmd="$BASE/md5deep $S  -p512 -r /tmp/test                     " ;;
    19) cmd="$BASE/sha1deep $S -p512 -r /tmp/test                     " ;;
    20) cmd="$BASE/sha256deep $S -r /tmp/test                      " ;;
    21) cmd="$BASE/tigerdeep $S  -r /tmp/test                      " ;;
    22) cmd="$BASE/whirlpooldeep $S -r /tmp/test                   " ;;
    23) cmd="$BASE/hashdeep $S -m -k hashlist-hashdeep-partial.txt /tmp/test/*.txt  " ;;
    24) cmd="$BASE/hashdeep $S -M -k hashlist-hashdeep-partial.txt /tmp/test/*.txt  " ;;
    25) cmd="$BASE/hashdeep $S -w -m -k hashlist-hashdeep-partial.txt /tmp/test/*.txt  " ;;
    26) cmd="$BASE/hashdeep $S -x -k hashlist-hashdeep-partial.txt /tmp/test/*.txt  " ;;
    27) cmd="$BASE/hashdeep $S -x -w -k hashlist-hashdeep-partial.txt /tmp/test/*.txt  " ;;
    28) cmd="$BASE/hashdeep $S -r -a -k hashlist-hashdeep-full.txt /tmp/test " ;;
    29) cmd="$BASE/hashdeep $S -v -r -a -k hashlist-hashdeep-full.txt /tmp/test "  ;;
    30) cmd="echo README.txt | $BASE/hashdeep"       ;;
    31) cmd="echo README.txt | $BASE/md5deep "      ;;
    32) cmd="echo README.txt | $BASE/sha1deep "      ;;
    33) cmd="echo README.txt | $BASE/sha256deep "    ;;
    34) cmd="echo README.txt | $BASE/whirlpooldeep " ;;
    35) cmd="$BASE/md5deep $S -Z /tmp/test/*.txt "      ;; 
    36) cmd="$BASE/md5deep $S -m  ../testfiles/known.txt .svn" ;;	  
    37) cmd="$BASE/md5deep $S -Sm ../testfiles/known.txt .svn" ;;	  
    38) cmd="$BASE/md5deep $S -sm ../testfiles/known.txt .svn" ;;
    39) cmd="$BASE/md5deep    /dev/null ";;
    40) cmd="$BASE/hashdeep   /dev/null ";;
   esac
   if [ x"$cmd" = "x" ]
   then 
     break
   fi
   $cmd | sed s+$BASE/++ | sed "s/-j0 //" > /tmp/test$i.out
   if [ $mode = "generate" ]
   then
     echo ""
     cp /tmp/test$i.out test$i.out
   fi
   if [ $mode = "test" ]
   then
     if ! diff /tmp/test$i.out test$i.out ; 
     then 
       echo TEST $i FAILED.
       echo COMMAND: $cmd
       ((fails++))
     else
       echo passes.
     fi
   fi
  done
done
echo Total Failures: $fails
exit $fails
  
  