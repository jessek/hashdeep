#!/bin/sh
# TEST_DIR is where the executable is that we are testing:
TEST_DIR=../hashdeep/
# GOOD_DIR is the executable that is known to be good
GOOD_DIR=/opt/local/bin/

echo Installing test files in /tmp/test
/bin/rm -rf /tmp/test
mkdir /tmp/test
cp -p ../testfiles/* /tmp/test

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
  for ((i=1;$i<23;i++))
  do 
   /bin/echo -n Test $i ...
   /bin/rm -f /tmp/test$i.out
   case $i in
     1) $BASE/md5deep -r    /tmp/test                      > /tmp/test$i.out ;; 
     2) $BASE/md5deep -p512 /tmp/test/deadbeef.txt         > /tmp/test$i.out ;;
     3) $BASE/md5deep -zr   /tmp/test                      > /tmp/test$i.out ;;
     4) $BASE/md5deep -b    /tmp/test/deadbeef.txt         > /tmp/test$i.out ;;
     5) $BASE/md5deep -m hashlist-md5deep.txt -r /tmp/test > /tmp/test$i.out ;;
     6) $BASE/md5deep -x hashlist-md5deep.txt -r /tmp/test > /tmp/test$i.out ;;
     7) $BASE/md5deep -M hashlist-md5deep.txt -r /tmp/test > /tmp/test$i.out ;;
     8) $BASE/md5deep -X hashlist-md5deep.txt -r /tmp/test > /tmp/test$i.out ;;
     9) $BASE/md5deep -m hashlist-md5deep.txt -w -r /tmp/test > /tmp/test$i.out ;;
    10) $BASE/md5deep -m hashlist-md5deep.txt -n -r /tmp/test > /tmp/test$i.out ;;
    11) $BASE/md5deep -w -a deadbeefb303ba89ae055ad0234eb7e8  -r /tmp/test > /tmp/test$i.out ;;
    12) $BASE/md5deep -w -A deadbeefb303ba89ae055ad0234eb7e8  -r /tmp/test > /tmp/test$i.out ;;
    13) $BASE/md5deep -k    /tmp/test/deadbeef.txt         > /tmp/test$i.out ;;
    14) $BASE/md5deep -i1000 -r /tmp/test                  > /tmp/test$i.out ;;
    15) $BASE/md5deep -I1000 -r /tmp/test                  > /tmp/test$i.out ;;
    16) $BASE/hashdeep -r   /tmp/test                      > /tmp/test$i.out ;;
    17) $BASE/hashdeep -p512 -r /tmp/test                    > /tmp/test$i.out ;;
    18) $BASE/md5deep  -p512 -r /tmp/test                    > /tmp/test$i.out ;;
    19) $BASE/sha1deep -p512 -r /tmp/test                    > /tmp/test$i.out ;;
    20) $BASE/sha256deep -r /tmp/test                     > /tmp/test$i.out ;;
    21) $BASE/tigerdeep  -r /tmp/test                     > /tmp/test$i.out ;;
    22) $BASE/whirlpooldeep -r /tmp/test                  > /tmp/test$i.out ;;
   esac
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
       ((fails++))
     else
       echo passes.
     fi
   fi
  done
done
echo Total Failures: $fails
exit $fails
  
  