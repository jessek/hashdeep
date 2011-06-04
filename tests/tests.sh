#!/bin/sh
# set GENERATE=yes to generate the outputs of the tests
DIR=/opt/local/bin/
GENERATE=no
/bin/rm -rf /tmp/test
mkdir /tmp/test
cp -p ../testfiles/* /tmp/test
for ((i=1;$i<16;i++))
do 
 /bin/echo -n Test $i ...
 /bin/rm -f /tmp/test$i.out
 case $i in
 1) $DIR/md5deep -r    /tmp/test                      > /tmp/test$i.out ;; 
 2) $DIR/md5deep -p512 /tmp/test/deadbeef.txt         > /tmp/test$i.out ;;
 3) $DIR/md5deep -zr   /tmp/test                      > /tmp/test$i.out ;;
 4) $DIR/md5deep -b    /tmp/test/deadbeef.txt         > /tmp/test$i.out ;;
 5) $DIR/md5deep -m hashlist-md5deep.txt -r /tmp/test > /tmp/test$i.out ;;
 6) $DIR/md5deep -k    /tmp/test/deadbeef.txt         > /tmp/test$i.out ;;
 7) $DIR/md5deep -i1000 -r /tmp/test                  > /tmp/test$i.out ;;
 8) $DIR/md5deep -I1000 -r /tmp/test                  > /tmp/test$i.out ;;
 9) $DIR/hashdeep -r   /tmp/test                      > /tmp/test$i.out ;;
 10) $DIR/hashdeep -p512 -r /tmp/test                    > /tmp/test$i.out ;;
 11) $DIR/md5deep  -p512 -r /tmp/test                    > /tmp/test$i.out ;;
 12) $DIR/sha1deep -p512 -r /tmp/test                    > /tmp/test$i.out ;;
 13) $DIR/sha256deep -r /tmp/test                     > /tmp/test$i.out ;;
 14) $DIR/tigerdeep  -r /tmp/test                     > /tmp/test$i.out ;;
 15) $DIR/whirlpooldeep -r /tmp/test                  > /tmp/test$i.out ;;

 esac
 if [ $GENERATE = "yes" ]
 then
   cp /tmp/test$i.out test$i.out
 fi
 if ! diff test$i.out /tmp/test$i.out ; 
 then 
   echo TEST $i FAILED.
   exit 1
 else
   echo passes.
 fi
done
