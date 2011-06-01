#!/bin/sh
/bin/rm -rf /tmp/test11 /tmp/test11.out
mkdir /tmp/test11
cp ../testfiles/* /tmp/test11
hashdeep -r /tmp/test11 > /tmp/test11.out
if ! diff test11.out /tmp/test11.out ; 
then 
  echo test11 failed.
  exit 1
else
  exit 0
fi
