
#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <mutex>
#include "SysInfo.h"
#include "kmeans.h"
#include "runnable.h"


class Scheduler :public Runnable{
private:
	SysInfo &sysinfo;						//system info class
	const static int minDistance = 10;		//the min distance needed between two clusters
	kmeans *k;								//kmeans algorithm
	int nodeTypeCount[SysInfo::MAX_NODES_NUM][3];	//count per type per node
	std::mutex *mu;
public:
	Scheduler(SysInfo &sysinfo,std::mutex &m);
	~Scheduler();
	static Scheduler* getInstance(SysInfo &sysinfo,std::mutex &m);	 //return an instance of class Scheduler
	void run();
	void runScheduler();					//run the main function of scheduler
	void classifyDomians();					//classify domains into "S,T,D"
	void assignTypes();						//assign types to each node
	void scheduleDomains();					//assign each domain to best selected node
	int getBestNode(int domain_i,int classType);			//get best node for domain i
	bool assignDomain2Node(int domain_i,int node_i);//assign domain i to node i

};
#endif