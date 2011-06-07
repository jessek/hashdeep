#!/bin/sh
# set GENERATE=yes to generate the outputs of the tests
# Run with tests.sh <base> to test programs in <base>
DEFAULT_BASE=/opt/local/bin/
GENERATE=yes

if [ x$1 != "x" ]; then
  BASE=$1
else
  BASE=$DEFAULT_BASE
fi

/bin/rm -rf /tmp/test
mkdir /tmp/test
cp -p ../testfiles/* /tmp/test
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
 if [ $GENERATE = "yes" ]
 then
   cp /tmp/test$i.out test$i.out
 fi
 if ! diff test$i.out /tmp/test$i.out ; 
 then 
   echo TEST $i FAILED.
   ((fails++))
 else
   echo passes.
 fi
done
echo Total Failures: $fails
exit $fails

