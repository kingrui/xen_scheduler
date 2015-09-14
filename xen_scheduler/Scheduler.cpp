#include "Scheduler.h"
#include <iostream>
using namespace std;

Scheduler::Scheduler(SysInfo &sysinfo,std::mutex &m) :sysinfo(sysinfo){
	mu = &m;
	k = new kmeans(3,minDistance);
}

Scheduler::~Scheduler(){
}

void Scheduler::run(){
	while(true){
		mu->lock();
		runScheduler();
		mu->unlock();
		this_thread::sleep_for(chrono::seconds(50));
	}
}


Scheduler* Scheduler::getInstance(SysInfo &sysinfo,std::mutex &m){
	static Scheduler sch(sysinfo,m);
	return & sch;
}    

void Scheduler::runScheduler(){
	classifyDomians();
	assignTypes();
	scheduleDomains();
}

/*
	classify domains into "S,T,D", using kmeans algrithm
	jinrui@jinrui.name
	2013-08-06
*/
void Scheduler::classifyDomians(){
	int domainCount =  sysinfo.domainCount;
	int nodeCount =  sysinfo.nodeCount;
	node  *domainNode[sysinfo.MAX_DOMS_NUM];
	double v[sysinfo.MAX_DOMS_NUM][2];

	//generate node for each domain
	for(int i=0;i<domainCount;i++){
		v[i][0] = 0;
		for(int j=0;j<nodeCount;j++){
			v[i][0]+= sysinfo.C[i][j];
		}
		v[i][1] = sysinfo.MissRate[sysinfo.dom_active_id_list[i]];//first get the id of domain i, then get the missrate using domain id
		domainNode[i] = new node(v[i]);
	}

	k->runKmeans(domainNode,sysinfo.type,domainCount);

	for (int i = 0; i < domainCount; i++){
		cout<<"domain"<<i<<":"<<sysinfo.type[i] << endl;
	}
	//getchar();

}

/*
	assign "S,T,D" to each node
	jinrui@jinrui.name
	2013-08-06
*/
void Scheduler::assignTypes(){
	int typeCount[3];
	for(int i=0;i<3;i++){
		typeCount[i] = 0;
	}
	for(int i=0;i<sysinfo.domainCount;i++){
		typeCount[sysinfo.type[i]]++;
	}
	
	int *orderedType = new int[sysinfo.domainCount];	//store ordered type,likes "S,S,S,D,D,D,T,T,T"
	int cursor = 0;
	for(int i=2;i>=0;i--){
		for(int j=0;j<typeCount[i];j++){
			orderedType[cursor++] = i;
		}
	}
	
	//assign types to each node ,likes node1"SSDT",node2"SDT",node3"SSD"...
	int cursorLeft = 0;
	int cursorRight = sysinfo.domainCount-1;
	while(cursorLeft<=cursorRight){
		for(int i=0;i<sysinfo.nodeCount;i++){
			if(cursorLeft<=cursorRight){
				nodeTypeCount[i][orderedType[cursorLeft++]]++;
			}
			else break;
		}
		for(int i=sysinfo.nodeCount-1;i>=0;i--){
			if(cursorLeft<=cursorRight){
				nodeTypeCount[i][orderedType[cursorRight--]]++;
			}
			else break;
		}
	}
}



/*
	assign each domain to best selected node
	jinrui@jinrui.name
	2013-08-07
*/
void Scheduler::scheduleDomains(){

	//i=2,1,0 corresponding to type S,D,T
	//firstly, assign type S, then D, then type T
	for(int i=2;i>=0;i--){
		for(int j=0;j<sysinfo.domainCount;j++){
			/*
			debug
			*/
			/*
			std::cout<<"domainCount:"<<sysinfo.domainCount<<endl;
			std::cout<<j<<endl;
			std::cout<<"nodeCount::"<<sysinfo.nodeCount<<endl;
			getchar();
			*/

			if(sysinfo.type[j]==i){
				int node = getBestNode(j,i);
				if(assignDomain2Node(j,node)){
					nodeTypeCount[node][i]--;	//set the count of type i of node to count-1
				}

			}
		}
	}
}

/*
	get best node for domain i
	jinrui@jinrui.name
	2013-08-20
*/
int Scheduler::getBestNode(int domain_i,int classType){
	bool hasMin = false;
	int minCost;
	int minNode;
	for(int i=0;i<sysinfo.nodeCount;i++){
		if(nodeTypeCount[i][classType]>0){
			if(!hasMin){
				minCost = sysinfo.calc_cost(domain_i,i);
				minNode = i;
				hasMin = true;
			}
			else{
				int thisCost = sysinfo.calc_cost(domain_i,i);
				if(thisCost<minCost){
					minCost = thisCost;
					minNode = i;
				}
			}
		}
	}
	return minNode;
}

/*
	assign domain i to node i
	jinrui@jinrui.name
	2013-08-09
*/
//TODO assign domain to the best selected node.
bool Scheduler::assignDomain2Node(int domain_i,int node_i){
	//migrate the domain to the cpupool which has least cost
	char cmd_msg[SysInfo::MAX_CHARS_ONE_LINE];
	int domain_id = sysinfo.dom_active_id_list[domain_i];
	sprintf(cmd_msg, "xl cpupool-migrate  %d Pool-node%d", domain_id, node_i);
	printf("%s\n",cmd_msg);
	system(cmd_msg);
	return true;
}
