import re
import os

#get node count
xl_info_lines = os.popen("xl info -n").readlines()
nr_nodes_pattern = re.compile(r'nr_nodes\s+:\s*([0-9]+)')
for line in xl_info_lines:
	match = nr_nodes_pattern.match(line)
	if match:
		node_count = int(match.group(1))
		break

#get pool 0 name and cpupool count
cpupool_list = os.popen("xl cpupool-list").readlines()
cpupool_count = len(cpupool_list)-1
pool0_name =  re.split('\s+',cpupool_list[1])[0]

#rename pool 0
os.popen("xl cpupool-rename "+pool0_name+" Pool-node0")
pool0_name = "Pool-node0"

cpu_node_map = dict()

#split
if cpupool_count < node_count:
	topology_pattern = re.compile(r'^\s*([0-9]+):\s+[0-9]+\s+[0-9]+\s+([0-9]+)$')
	for line in xl_info_lines:
		match = topology_pattern.match(line)
		if match:
			cpu_node_map[int(match.group(1))] = int(match.group(2))
	#create cpu pool
	for i in range(1,node_count):
		os.popen("xl cpupool-create cpupool_cfg")
		os.popen("xl cpupool-rename Pool-node-default Pool-node"+str(i))

#leave the key = 0 cpu in cpupool 0
	for key in cpu_node_map.keys():
		if key != 0:
			os.popen("xl cpupool-cpu-remove "+pool0_name+" "+str(key))

	for key in cpu_node_map.keys():
		if cpu_node_map[key] == 0:
			if key != 0:
				os.popen("xl cpupool-cpu-add "+pool0_name+" "+str(key))
		else:
			os.popen("xl cpupool-cpu-add Pool-node"+str(cpu_node_map[key])+" "+str(key))

