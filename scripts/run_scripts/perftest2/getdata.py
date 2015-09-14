#!/usr/bin/env python

file = open('summary.res_1029_2_modified')
i = 0
count = 0
total = 0.0
v=[0.0,0.0,0.0,0.0,0.0]
m=[0.0,0.0,0.0,0.0,0.0]
for line in file:
	if(i%5!=0):
		v[i%5]= v[i%5]+float(line)	
		if(float(line)>m[i%5]):
			m[i%5] = float(line)
	else:
		count = count+1
	i=i+1
        #print i, ":", line
for x in v[1:]:
	print x/count
	total = total + x/count
print count
print v[1:]
print m[1:]
print total
file.close()
