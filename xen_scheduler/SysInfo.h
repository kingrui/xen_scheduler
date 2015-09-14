#ifndef SYSINFO_H_
#define SYSINFO_H_

#include <libvirt/libvirt.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <mutex>
#include "runnable.h"


class SysInfo :public Runnable
{
private:
	int get_numa_node_nr();
	int get_pcpus_nr();
	int getNumaInfo(char* searchText);
	int get_node_distance_matrix();
	int getCpuMatrix(int i);
	void getMemoryMatrix(int i,int dom_id);
	int parse_domain_mem_alloc(int dom_id);
	int getNodeCpuCount(void);
	std::mutex *mu;
	
public:

	static const int ERROR = 1;
	static const int SUCCESS = 0;
	static const int MAX_DOMS_NUM = 1024;
	static const int MAX_CPUS_NUM = 128;
	static const int BITS_IN_ONE_BYTES = 8;
	static const int MEM_THRESHOLD = 200*1024;
	static const int MAX_NODES_NUM = 16;
	static const int MAX_CHARS_ONE_LINE = 80;
	static const int CPU_THRESHOLD = 0;
	static const int SLEEP_INTERVAL = 1;
	static const int MAX_VCPU_IN_DOMAINS = 8;
	static const int MAX_LEN_FILE_NAME = 64;
	static const int MAX_FILE_BUFF_SIZE = 64;
	static const double WEIGHT_OF_CPU_COST;

	float C[MAX_DOMS_NUM][MAX_NODES_NUM];		//Cpu usage per domain node, normalized
	float CPU[MAX_DOMS_NUM][MAX_NODES_NUM];		//Cpu usage per domain node
	float M[MAX_DOMS_NUM][MAX_NODES_NUM];		//memory uage per domain node,normalized
	float MEM[MAX_DOMS_NUM][MAX_NODES_NUM];		//memory uage per domain node
	float MissRate[MAX_DOMS_NUM];				//the cache miss rate of each domain
	float cost[MAX_DOMS_NUM][MAX_NODES_NUM];	//the cost of each domain in each node
	int type[MAX_DOMS_NUM];						//store domain type S,T,D
	int node_dist_matrix[MAX_NODES_NUM][MAX_NODES_NUM];
	int dom_mem_alloc[MAX_NODES_NUM];
	virConnectPtr conn;
	virDomainPtr domainPtrList[MAX_DOMS_NUM];
	int dom_active_id_list[MAX_DOMS_NUM];
	int domainCount;
	int nodeCount;
	int nodeCpuCount[MAX_NODES_NUM];
	int allRunningDomainsNum;
	

	SysInfo(std::mutex &m);
	~SysInfo(void);

	static SysInfo* getInstance(std::mutex &m);
	void run();

	int getActiveDomian();
	double domain_get_cpu_util(virDomainPtr domain_p);
	//int scheduler();
	int getDomainsInfo();						//get domains' CPU info,memory info
	int getMissRate();							//get the missrate of domains
	int calc_cost(int domain_i,int which_node_to_cal);//get the cost of each domain in erver node
};

#endif
