#include "Vnuma.h"
#include <iostream>
using namespace std;

Vnuma::Vnuma(SysInfo &sysinfo,std::mutex &m) :sysinfo(sysinfo){
	mu = &m;
}

void Vnuma::run(void){
	while(true){
		mu->lock();
		runVnumaScheduler();
		mu->unlock();
		this_thread::sleep_for(chrono::seconds(60));
	}
}

Vnuma* Vnuma::getInstance(SysInfo &sysinfo,std::mutex &m){
	static Vnuma vnuma(sysinfo,m);
	return &vnuma;

}

//main scheduler function
//jinrui
//jinrui@jinrui.name
//2013.10.12
void Vnuma::runVnumaScheduler(void){
	cout<<"vnuma running..."<<endl;
	

	int nodecount = sysinfo.nodeCount;
	int domaincount = sysinfo.domainCount;
    double *nodeload = new double[nodecount];

	 //calculate node load
	for(int i=0;i<nodecount;i++){
		double cpu_usage = 0;
		double mem_usage = 0;
        for(int j=0;j<domaincount;j++){
            cpu_usage += sysinfo.CPU[j][i];
			mem_usage += sysinfo.MEM[j][i];
        }
		cpu_usage = cpu_usage/1000000000/sysinfo.nodeCpuCount[i];
		if(cpu_usage>0.9){
			cpu_usage = 0.9;
		}
		mem_usage = mem_usage/256;
		cout<<"node"<<i<<": CPU:"<<cpu_usage<<endl;
		cout<<"node"<<i<<": MEM:"<<mem_usage<<endl;

		nodeload[i] = (1 / (1 - cpu_usage)) *mem_usage;
    }
	for(int i=0;i<nodecount;i++){
		cout<<"nodeload["<<i<<"]:"<<nodeload[i]<<endl;
	}


    
    //find min and max node
    int max = 0;
    int min = 0;
    for(int i=1;i<nodecount;i++){
        if(nodeload[i]>nodeload[max]){
            max = i;
        }
        else if(nodeload[i]<nodeload[min]){
            min = i;
        }
    }

    cout<<"min node is "<<min<<":"<<nodeload[min]<<",max node is "<<max<<":"<<nodeload[max]<<endl;
	if(nodeload[max]-nodeload[min]>THRESHOLD){
		cout<<"a migration is needed"<<endl;
		doMigration(max,min);
	}

	cout<<"vnuma scheduler done..."<<endl;
}

//calculate the load of domain
//jinrui
//jinrui@jinrui.name
//2013.10.12
double Vnuma::calc_laod(double cpu_usage,double mem_usage){
	return cpu_usage+mem_usage;
}

//do the migration
//jinrui
//jinrui@jinrui.name
//2013.10.12
void Vnuma::doMigration(int sourceNode,int targetNode){
	int bestDomain = -1;
	double maxload = 0;
	for(int i=0;i<sysinfo.domainCount;i++){
		double thisload = sysinfo.CPU[i][sourceNode] * sysinfo.MEM[i][sourceNode];
		if(thisload>maxload){
			maxload = thisload;
			bestDomain = i;
		}
	}
	if(bestDomain!=-1){
		char cmd_msg[SysInfo::MAX_CHARS_ONE_LINE];
		int domain_id = sysinfo.dom_active_id_list[bestDomain];
		sprintf(cmd_msg, "xl cpupool-migrate  %d Pool-node%d && xl vcpu-pin -M %d all %d", domain_id, targetNode,domain_id,targetNode);
		//xl vcpu-pin -M domain_id all 2
		printf("%s\n",cmd_msg);
		system(cmd_msg);
	}

	return;
}

Vnuma::~Vnuma(void)
{
}
