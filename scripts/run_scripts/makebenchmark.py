#!/usr/bin/env python
import os,sys
if(len(sys.argv)<4 or sys.argv[1] in ("-h","-help")):
	print "usage: {0} benchmark NRPROCS CLASS ".format(sys.argv[0])
	sys.exit()

names = range(202,203)
for name in names:
	cmdstr ="ssh root@192.168.1.{0} \"cd /root/NPB3.3/NPB3.3-MPI; make {1} NPROCS={2} CLASS={3}\"".format(name,sys.argv[1],sys.argv[2],sys.argv[3])
	print cmdstr
	os.popen(cmdstr)
