#!/bin/sh
# TEST_DIR is where the executable is that we are testing:
TEST_DIR=../hashdeep
# GOOD_DIR is the executable that is known to be good
GOOD_DIR=/opt/local/bin

echo Installing test files in /tmp/test
/bin/rm -rf /tmp/test
mkdir /tmp/test
cp -p ../testfiles/* /tmp/test

echo Creating hashlist files if they do not exist

if [ ! -r hashlist-hashdeep.txt ] ;
then
  hashdeep -l /tmp/test/deadbeef.txt  /tmp/test/foo.txt > hashlist-hashdeep.txt
  tail -1 hashlist-hashdeep.txt | sed s+/tmp/test/foo.txt+/no/match/em+ | sed s/[012345]/6/g >> hashlist-hashdeep.txt
fi

for mode in generate test
do
  echo $mode mode.
  if [ $mode = "generate" ] 
  then
    BASE=$GOOD_DIR
  else
    BASE=$TEST_DIR
  fi
  fails=0
  for ((i=1;$i<=27;i++))
  do 
   /bin/echo -n Test $i ...
   /bin/rm -f /tmp/test$i.out
   case $i in
     1) cmd="$BASE/md5deep -r    /tmp/test                       " ;;
     2) cmd="$BASE/md5deep -p512 /tmp/test/deadbeef.txt          " ;;
     3) cmd="$BASE/md5deep -zr   /tmp/test                       " ;;
     4) cmd="$BASE/md5deep -b    /tmp/test/deadbeef.txt          " ;;
     5) cmd="$BASE/md5deep -m hashlist-md5deep.txt -r /tmp/test  " ;;
     6) cmd="$BASE/md5deep -x hashlist-md5deep.txt -r /tmp/test  " ;;
     7) cmd="$BASE/md5deep -M hashlist-md5deep.txt -r /tmp/test  " ;;
     8) cmd="$BASE/md5deep -X hashlist-md5deep.txt -r /tmp/test  " ;;
     9) cmd="$BASE/md5deep -m hashlist-md5deep.txt -w -r /tmp/test  " ;;
    10) cmd="$BASE/md5deep -m hashlist-md5deep.txt -n -r /tmp/test  " ;;
    11) cmd="$BASE/md5deep -w -a deadbeefb303ba89ae055ad0234eb7e8  -r /tmp/test  " ;;
    12) cmd="$BASE/md5deep -w -A deadbeefb303ba89ae055ad0234eb7e8  -r /tmp/test  " ;;
    13) cmd="$BASE/md5deep -k    /tmp/test/deadbeef.txt          " ;;
    14) cmd="$BASE/md5deep -i1000 -r /tmp/test                   " ;;
    15) cmd="$BASE/md5deep -I1000 -r /tmp/test                   " ;;
    16) cmd="$BASE/hashdeep -r   /tmp/test                       " ;;
    17) cmd="$BASE/hashdeep -p512 -r /tmp/test                     " ;;
    18) cmd="$BASE/md5deep  -p512 -r /tmp/test                     " ;;
    19) cmd="$BASE/sha1deep -p512 -r /tmp/test                     " ;;
    20) cmd="$BASE/sha256deep -r /tmp/test                      " ;;
    21) cmd="$BASE/tigerdeep  -r /tmp/test                      " ;;
    22) cmd="$BASE/whirlpooldeep -r /tmp/test                   " ;;
    23) cmd="$BASE/hashdeep -m -k hashlist-hashdeep.txt /tmp/test/*.txt  " ;;
    24) cmd="$BASE/hashdeep -M -k hashlist-hashdeep.txt /tmp/test/*.txt  " ;;
    25) cmd="$BASE/hashdeep -w -m -k hashlist-hashdeep.txt /tmp/test/*.txt  " ;;
    26) cmd="$BASE/hashdeep -x -k hashlist-hashdeep.txt /tmp/test/*.txt  " ;;
    27) cmd="$BASE/hashdeep -x -w -k hashlist-hashdeep.txt /tmp/test/*.txt  " ;;
   esac
   $cmd | sed s+$BASE/++ > /tmp/test$i.out
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
  
  