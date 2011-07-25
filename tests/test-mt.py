#/usr/bin/env python
#
# This script runs multiple iterations from a directory

from optparse import OptionParser
global options
import sys,time
from subprocess import Popen,PIPE,call

threads = [0,1,2,4,8]

if __name__=="__main__":
    parser = OptionParser()
    parser.usage = """usage: %prog [options] <dir>"""
    (options,args) = parser.parse_args()
    if len(args)!=1:
        parser.print_help()
        exit(1)
    
    root = args[0]
    for thread in threads:
        print("Trying %d threads" % thread)
        n1 = "mt-out.stdout." + str(thread)
        n2 = "mt-out.stderr." + str(thread)
        t0 = time.time()
        r = call(["../src/hashdeep","-j" + str(thread),"-r",root],
                  stdout=open(n1,"w"),
                  stderr=open(n2,"w"))
        t1 = time.time()
        t = t1-t0
        print(" ... %d threads return code = %d time=%f" % (thread,r,t))
        
        
    
    
