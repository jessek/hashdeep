#!/bin/sh
GENERATE=yes
GENERATE=no
/bin/rm -rf /tmp/test
mkdir /tmp/test
cp -p ../testfiles/* /tmp/test
for i in 1 2 3 4 5 6 7 8 9 
do 
 /bin/rm -f /tmp/test$i.out
 case $i in
 1) md5deep -r    /tmp/test                      > /tmp/test$i.out ;; 
 2) md5deep -p512 /tmp/test/deadbeef.txt         > /tmp/test$i.out ;;
 3) md5deep -zr   /tmp/test                      > /tmp/test$i.out ;;
 4) md5deep -b    /tmp/test/deadbeef.txt         > /tmp/test$i.out ;;
 5) md5deep -m hashlist-md5deep.txt -r /tmp/test > /tmp/test$i.out ;;
 6) md5deep -k    /tmp/test/deadbeef.txt         > /tmp/test$i.out ;;
 7) md5deep -i1000 -r /tmp/test                  > /tmp/test$i.out ;;
 8) md5deep -I1000 -r /tmp/test                  > /tmp/test$i.out ;;
 9) hashdeep -r   /tmp/test                      > /tmp/test$i.out ;;
 esac
 if [ $GENERATE = "yes" ]
 then
   cp /tmp/test$i.out test$i.out
 fi
 if ! diff test$i.out /tmp/test$i.out ; 
 then 
   echo test $i failed.
   exit 1
 else
   echo test $i passes.
 fi
done
