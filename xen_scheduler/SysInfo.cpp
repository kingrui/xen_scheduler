/*
jinrui
*/

#include "SysInfo.h"
#include <iostream>
using namespace std;


/*
 *The constructor of SysInfo
 *Jin Rui
 *jinrui@jinrui.name
 */
SysInfo::SysInfo(mutex &m)
{
	mu = &m;

	//set an initial connection use libvirt
	if ((conn = virConnectOpen("xen:///"))== NULL){
		perror("failed to execute virConnectOpen function");
		return;
	}
	//system("xl cpupool-numa-split");					//split the domains to cpupool
	system("python cpupool-numa-split.py");	

	memset(domainPtrList, 0, sizeof(domainPtrList));	//initialize the domain pointer list
	domainCount = 0;									//set domain count to 0
	nodeCount = get_numa_node_nr();						//get the node number of system
	get_node_distance_matrix();							//get the cpu distance matrix


	getNodeCpuCount();									//get the node cpu count
	memset(dom_mem_alloc, 0, sizeof dom_mem_alloc);
}

int SysInfo::getNodeCpuCount(void){
	FILE *file_p;
	if((file_p = fopen("node_info.cfg", "r")) == NULL){
		perror("failed to open file 'result2'");
		return ERROR;
	}

	int node_id;
	int cpu_count;
	for(int i=0;i<nodeCount;i++){
		fscanf(file_p,"%d",&node_id);
		fscanf(file_p,"%d",&cpu_count);
		nodeCpuCount[node_id] = cpu_count;
		cout<<"node"<<node_id<<": "<<cpu_count<<endl;
	}

	fclose(file_p);

	return SUCCESS;
}


SysInfo::~SysInfo(void)
{
	virConnectClose(conn);								//close the connection of libvirt
}

double const SysInfo::WEIGHT_OF_CPU_COST = 1.0;


/*
 *return an instance of SysInfo
 *Jin Rui
 *jinrui@jinrui.name
 *2013-08-08
 */
SysInfo* SysInfo::getInstance(std::mutex &m){
	static SysInfo sys(m);
	return &sys;
}

void SysInfo::run(){
	while(true){
		mu->lock();
		getActiveDomian();								//get the domains list that need schedule
		getDomainsInfo();								//get the information of domians
		mu->unlock();
		this_thread::sleep_for(chrono::seconds(10));	//sleep for ten seconds
	}
	
}

/*
 *get the domains list that need schedule
 *Jin Rui
 *jinrui@jinrui.name
 *2013-08-08
 */
int SysInfo::getActiveDomian(){
	int totalDomainCount = 0;
	virDomainPtr domsPtr_total_list[MAX_DOMS_NUM];
	/*
	 * 	get the active domain, store the domian_ptr in doms_active_Ptr_lst[i];
	 */
	totalDomainCount = virConnectNumOfDomains(conn);//the number of domain found or -1 in case of error
	this->allRunningDomainsNum = totalDomainCount; //save the all running domains number to allRunningDomainsNum
	
	if(totalDomainCount < 0){
		perror("failed to execute virConnectNumOfDomains");
		return ERROR;
	}

	totalDomainCount = virConnectListDomains(conn, dom_active_id_list, totalDomainCount);//dom_active_id_lst array to collect the list of IDs of active domains
	if(totalDomainCount < 0){
		perror("failed to execute virConnectListDomains");
		return ERROR;
	}

	//get all the domain pointer by domain id
	for(int i = 0; i < totalDomainCount; i++){
		domsPtr_total_list[i] = virDomainLookupByID(conn, dom_active_id_list[i]);//get domain pointer by id
		if(domsPtr_total_list[i] == NULL){
			perror("failed to execute virDomainLookupByID");
			return ERROR;
		}

	}

	/*
	 * 	query every domains's used memory
	 * 	set domsPtr_active_lst[i] = NULL,
	 * 	if the domain's mem is less than the MEM_THREASHOLD.
	 */
	virDomainInfo domain_info;
	for(int i = 0; i < totalDomainCount; i++){
		if(virDomainGetInfo(domsPtr_total_list[i], &domain_info) < 0){
			perror("failed to virDomainGetInfo");
			return ERROR;
		}

		//DEBUG print every domain memory.
		cout<<"dom_mem_max : "<<domain_info.maxMem<<endl;
		cout<<"dom_mem_used : "<<domain_info.memory<<endl;

		//if the memory usage of a domain is less than threshold, free the domain pointer
		if(domain_info.memory < MEM_THRESHOLD){
			free(domsPtr_total_list[i]);
			domsPtr_total_list[i] = NULL;
		}
	}

	/*
	 * 	query every domain CPU total utilization.
	 * 	populate domsPtr_sche_lst
	 */
	domainCount= 0;
	double dom_cpu_uti = 0.0;

	/*
	for (int i = 0; i < totalDomainCount; i++){
		cout << "-dom_active_id_list[" << i << "]:" << dom_active_id_list[i] << endl;
	}
	*/

	for(int i = 0; i < totalDomainCount; i++){
		if(domsPtr_total_list[i] != NULL){
			if(dom_active_id_list[i]==0)//ignore domain0
			{
				continue;
			}

			dom_cpu_uti = domain_get_cpu_util(domsPtr_total_list[i]);
			cout<<"dom_cpu_utilization : "<<dom_cpu_uti<<endl;
			if(dom_cpu_uti > CPU_THRESHOLD){
				domainPtrList[domainCount] = domsPtr_total_list[i];
				dom_active_id_list[domainCount++] = dom_active_id_list[i];
			}
				
			else
				continue;
		}
	}
	
	for (int i = 0; i < domainCount; i++){
		cout << "dom_active_id_list[" << i << "]:" << dom_active_id_list[i] << endl;
		//getchar();
	}
	
	return SUCCESS;
}

double SysInfo::domain_get_cpu_util(virDomainPtr domain_p){
        virDomainInfo info;
        double cpu_time_fir; 		//cnt in ns
        double cpu_time_sec;		//cnt in ns
        struct timeval sys_time_fir;	//cnt in us
        struct timeval sys_time_sec;	//cnt in us
        double cpu_time_elap;		//cnt in us
        double sys_time_elap;		//cnt in us


        //first snapshot of CPU time and current_system_time;
        if ((virDomainGetInfo(domain_p, &info)) < 0)
                perror("failed virDomainGetInfo to get the CPU time");
        cpu_time_fir = (double)info.cpuTime;

        if (gettimeofday(&sys_time_fir, NULL) == -1)
                perror("failed to gettimeofday.");

        if(sleep(SLEEP_INTERVAL) != 0)
                perror("failed to sleep for a while");

         //second snapshot of CPU time and current_system_time;
        if ((virDomainGetInfo(domain_p, &info)) < 0)
                perror("failed virDomainGetInfo to get the CPU time");
        cpu_time_sec = (double)info.cpuTime;

        if (gettimeofday(&sys_time_sec, NULL) == -1)
                perror("failed to gettimeofday.");

        //calculate the CPU utilization.
        cpu_time_elap = (cpu_time_sec - cpu_time_fir) / 1000;
        sys_time_elap = 1000000 * (sys_time_sec.tv_sec - sys_time_fir.tv_sec) +
                        (sys_time_sec.tv_usec - sys_time_fir.tv_usec);
        return cpu_time_elap / (double)sys_time_elap;
}



/*
 *Get the number of node
 *Jin Rui
 *jinrui@jinrui.name
 *2013-07-08
 */
int SysInfo::get_numa_node_nr(){
	return getNumaInfo((char *)("xl info -n | grep nr_nodes > ./numa_info"));
}

/*
 *Get the number of cpus
 *Jin Rui
 *jinrui@jinrui.name
 *2013-07-08
 */
int SysInfo::get_pcpus_nr(){
	return getNumaInfo((char *)("xl info -n | grep nr_cpus > ./numa_info"));
}

/*
 *Help to get the number of nodes and cpus by a system call
 *Jin Rui
 *jinrui@jinrui.name
 *2013-07-08
 */
int SysInfo::getNumaInfo(char* searchText){
	int nodeNum = 0;
	char a[50];//save syscall result
	int i=0;
	
	//serchText is sth. like "xl -f info -n | grep nr_cpus > ./numa_info"
	if(system(searchText) < 0){
		perror("failed to execute system() to get xm info");
	}
	FILE *numa_info_file_p;
	if((numa_info_file_p = fopen("./numa_info", "r")) < 0){
		perror("failed to fopen the file numa_info.");
		return -1;
	}
	else{
		//get the number from the result like "nr_nodes               : 2"
		fgets(a,50,numa_info_file_p);
		while(i<50&&a[i] != '\0'){
			if(a[i]>='0'&&a[i]<='9'){
				nodeNum = nodeNum*10 + (a[i]-'0');
			}
			i++;
		}
	}
	fclose(numa_info_file_p);
    return nodeNum;
}

int SysInfo::get_node_distance_matrix()
{
	cout<<"starting to get_node_distance_matrix..."<<endl;
	//set default distance

	for(int i=0;i<nodeCount;i++)
		for(int j=0;j<nodeCount;j++){
			if(i==j)
				node_dist_matrix[i][j] = 10;
			else
				node_dist_matrix[i][j] = 20;
		}
	/*
    char cmd[MAX_CHARS_ONE_LINE];
	char file_name[MAX_LEN_FILE_NAME];
    char buff[MAX_FILE_BUFF_SIZE];
	sprintf(cmd, "xl info -n > numa_info");
    if(system(cmd) < 0){
        perror("failed to execute the xl info command");
        return ERROR;
    }
	FILE *file_p;
	if((file_p = fopen("numa_info", "r")) == NULL){
		perror("failed to open file 'numa_info'");
		return ERROR;
    }
	fgets(buff, MAX_FILE_BUFF_SIZE, file_p);
	while(!strstr(buff,"numa_info")){
		if(fgets(buff, MAX_FILE_BUFF_SIZE, file_p)==NULL)
			return ERROR;
	}
	fgets(buff, MAX_FILE_BUFF_SIZE, file_p);
	for(int i=0;i<nodeCount;i++){
		fgets(buff, MAX_FILE_BUFF_SIZE, file_p);
		cout<<buff;
		int p;
		int memsize=0;
		int memfree=0;
		for(p=0;p<MAX_FILE_BUFF_SIZE&&buff[p]!=':';p++);//skip domainid: 
		for(;p<MAX_FILE_BUFF_SIZE&&(buff[p]<'0'||buff[p]>'9');p++);//skip '     '
		for(;p<MAX_FILE_BUFF_SIZE&&(buff[p]>='0'&&buff[p]<='9');p++)//get memsize
		{
			memsize = memsize*10+buff[p]-'0';
		}
		cout<<"memsize:"<<memsize<<endl;
		for(;p<MAX_FILE_BUFF_SIZE&&(buff[p]<'0'||buff[p]>'9');p++);//skip '      '
		for(;p<MAX_FILE_BUFF_SIZE&&(buff[p]>='0'&&buff[p]<='9');p++)//get memfree
		{
			memfree = memfree*10+buff[p]-'0';
		}
		cout<<"memfree:"<<memfree<<endl;
		for(;p<MAX_FILE_BUFF_SIZE&&(buff[p]<'0'||buff[p]>'9');p++);//skip '      '
		for(int j=0;j<nodeCount;j++)//get node distance
		{
			int dis =0;
			for(;p<MAX_FILE_BUFF_SIZE&&(buff[p]>='0'&&buff[p]<='9');p++){
				dis = dis*10 + buff[p]-'0';
			}
			p++;
			node_dist_matrix[i][j] = dis;
			cout<<"dist["<<i<<"]["<<j<<"]="<<dis<<endl;
		}


	}
	fclose(file_p);    
	*/
	cout<<"finish to get_node_distance_matrix..."<<endl;
    return SUCCESS;
}


/*
 * Get domain Cpu usage per node(domain i)
 * jinrui
 * jinrui@jinrui.name
 * 2013-07-11
*/
int SysInfo::getCpuMatrix(int i){
	cout<<"running getCPUMatrix..."<<endl;
	virVcpuInfo vcpuinfo_lst[MAX_VCPU_IN_DOMAINS];
	virVcpuInfo vcpuinfo_lst2[MAX_VCPU_IN_DOMAINS];
	double CpuTime[MAX_VCPU_IN_DOMAINS];
	int kk=0;
	int cpu_node_map[MAX_CPUS_NUM];
    int nr_vcpus = MAX_VCPU_IN_DOMAINS;
    unsigned char *cpumaps;
	int j;

    int cpumap_len = get_pcpus_nr()/BITS_IN_ONE_BYTES;
    	
	for(kk=0;kk<nodeCount;kk++){
		cpu_node_map[kk] = -1;
	}
	system("xl debug-keys u");
	system("xl dmesg | grep \"(XEN) CPU[0-9]* -> NODE[0-9]*\" >mesgout");
	system("sed 's/(XEN) CPU//g' mesgout -i");
	system("sed 's/ -> NODE/ /g' mesgout -i");

	FILE *file_p;
	if((file_p = fopen("mesgout", "r")) == NULL){
		perror("failed to open file 'mesgout'");
        return ERROR;
	}
	int pcpuNum;
	int nodeNum;
	
	while(fscanf(file_p, "%d%d",&pcpuNum,&nodeNum)==2){
		//printf("debug-%d,%d\n",pcpuNum,nodeNum);
		cpu_node_map[pcpuNum] = nodeNum;
	}
	
	fclose(file_p);

    cpumaps = (unsigned char*)calloc(nr_vcpus, cpumap_len);

    nr_vcpus = virDomainGetVcpus(domainPtrList[i], vcpuinfo_lst, nr_vcpus, cpumaps, cpumap_len);

    if (nr_vcpus < 0){
            perror("failed to virDomainGetVcpus");
            exit(ERROR);
    }

    struct timeval sys_time_fir;	//cnt in us
    struct timeval sys_time_sec;	//cnt in us

	if (gettimeofday(&sys_time_fir, NULL) == -1)
            perror("failed to gettimeofday.");

    if(sleep(SLEEP_INTERVAL) != 0)
            perror("failed to sleep for a while");

		nr_vcpus = virDomainGetVcpus(domainPtrList[i], vcpuinfo_lst2, nr_vcpus, cpumaps, cpumap_len);
    if (nr_vcpus < 0){
            perror("failed to virDomainGetVcpus");
            exit(ERROR);
    }

	if (gettimeofday(&sys_time_sec, NULL) == -1)
            perror("failed to gettimeofday.");

	double sys_time_elap = 1000000 * (sys_time_sec.tv_sec - sys_time_fir.tv_sec) +
                    (sys_time_sec.tv_usec - sys_time_fir.tv_usec);

	cout<<"nr_vcpus:"<<nr_vcpus<<endl;
	
	float cpuTimeSum = 0;
	for(kk=0;kk<nr_vcpus;kk++){
		CpuTime[kk] = vcpuinfo_lst2[kk].cpuTime - vcpuinfo_lst[kk].cpuTime;
		cpuTimeSum += CpuTime[kk];
		cout<<"number:"<<vcpuinfo_lst2[kk].number<<",cpu:"<<vcpuinfo_lst2[kk].cpu<<endl;
	}

	for(kk=0;kk<nodeCount;kk++){
		C[i][kk]=0;
		CPU[i][kk]=0;
	}


	for(kk=0;kk<nr_vcpus;kk++){
		int nodeNum = cpu_node_map[vcpuinfo_lst2[kk].cpu];
		if(nodeNum>=0){
			C[i][nodeNum]+= (CpuTime[kk]/cpuTimeSum);
			CPU[i][nodeNum]+= CpuTime[kk];
		}
		//cout<<"Vcpu:"<<kk<<",cpu:"<<C[i][nodeNum]<<endl;
	}

	//
	for(kk=0;kk<nodeCount;kk++){
		cout<<"node"<<kk<<":normalized:"<<C[i][kk]<<endl;
		cout<<"node"<<kk<<":"<<CPU[i][kk]<<endl;
	}

	free(cpumaps);
	
	cout<<"finish getCPUMatrix..."<<endl;
	return SUCCESS;
}


/*
 * Get domain's memory distribution(domian i,domain id is dom_id)
 * jinrui
 * jinrui@jinrui.name
 * 2013-07-11
 */
void SysInfo::getMemoryMatrix(int i,int dom_id){
	cout<<"running getMemoryMatrix..."<<endl;
	
	char dmesg_cmd[MAX_CHARS_ONE_LINE];

    system("xl debug-keys u");
    sprintf(dmesg_cmd, "xl dmesg | tail -n %d > mem_distri", (nodeCount + 1) * domainCount);
    system(dmesg_cmd);

    
    if(parse_domain_mem_alloc(dom_id) < -1){
        perror("failed to parser the dom_mem_alloc");
        return ;
    }

	//get memory usage pesentage per node
	int memory_sum = 0;
	int kk=0;
	for(kk = 0; kk < nodeCount; kk++){
		memory_sum+=dom_mem_alloc[kk];
	}
	for(kk = 0; kk < nodeCount; kk++){
		M[i][kk]=dom_mem_alloc[kk]*1.0/memory_sum;
		MEM[i][kk]=dom_mem_alloc[kk];
	}
	for(kk = 0; kk < nodeCount; kk++){
		cout<<"memory-node"<<kk<<":"<<M[i][kk]<<endl;
	}
	cout<<"finish getMemoryMatrix..."<<endl;

	return ;

}


/*
 *Modified by
 *Jin Rui
 *jinrui@jinrui.name
 *2013-07-09
 */
int SysInfo::parse_domain_mem_alloc(int dom_id)
{
        FILE *file_p;
        char cmd[MAX_CHARS_ONE_LINE];
        char file_name[MAX_LEN_FILE_NAME];
        char buff[MAX_FILE_BUFF_SIZE];
        /*
         *      'sed' to grep the domain memory information
         *      store the into mem_info_DomX
         */
        sprintf(cmd, "cat mem_distri | sed -n '/Domain %d /,+%dp' | tail -n %d > mem_info_Dom%d", dom_id, nodeCount, nodeCount, dom_id);
        if(system(cmd) < 0){
                perror("failed to system the 'sed' to grep domainX");
                return ERROR;
        }

        sprintf(file_name, "mem_info_Dom%d", dom_id);
        if((file_p = fopen(file_name, "r")) == NULL){
                perror("failed to open file 'mem_info_Dom'");
                return ERROR;
        }
        for(int i = 0; i < nodeCount; i++){
                fgets(buff, MAX_FILE_BUFF_SIZE, file_p);
				dom_mem_alloc[i]=0;

				//buff="XEN    Node 0: 748374",get the finger from buff
				strtok(buff, ":");
				char *num_p = strtok(NULL, ":");//num_p=' 748374'

				int k=0;
				while(*(num_p+k)&&num_p[k] != '\0'){
					if(num_p[k]>='0'&&num_p[k]<='9'){
						dom_mem_alloc[i] = dom_mem_alloc[i]*10 + (num_p[k]-'0');
					}
					k++;
				}
				cout<<"dome_id:"<<dom_id<<",node:"<<i<<",mem_alloc:"<<dom_mem_alloc[i]<<endl;

        }
        fclose(file_p);

        sprintf(cmd, "rm mem_info_Dom%d", dom_id);
        if(system(cmd) < 0){
                perror("failed to system the 'rm' to remove temperate file");
                return ERROR;
        }

		
        return SUCCESS;

}


//the 2 dimensional array's second dimensional number must be assign. temporally assign it to 2
int SysInfo::calc_cost(int domain_i,int which_node_to_cal)
{
    int sum_cost = 0;
    for(int i = 0; i < nodeCount; i++){
        sum_cost += M[domain_i][i] * node_dist_matrix[which_node_to_cal][i];
    }
	sum_cost+= CPU[domain_i][which_node_to_cal]*WEIGHT_OF_CPU_COST;
    return sum_cost;
}

/*
 *get the domain information
 *Jin Rui
 *jinrui@jinrui.name
 *2013-07-09
 */
int SysInfo::getDomainsInfo(){
	cout<<"starting to getDomainsInfo"<<endl;
	
	memset(node_dist_matrix, 0, sizeof node_dist_matrix);

	int currentDomainCount = virConnectNumOfDomains(conn);//the number of domain found or -1 in case of error
	if(currentDomainCount!=this->allRunningDomainsNum){
		cout<<"Domains have been changed"<<endl;
		return ERROR;
	}

    for(int i = 0; i < domainCount; i++){
        int dom_id = dom_active_id_list[i];

		//Get domain's CPU distribution:
		getCpuMatrix(i);

		
        //Get domain's memory distribution:
		getMemoryMatrix(i,dom_id);
        
		//get the cost for each domain node
		for(int j=0;j<nodeCount;j++){
			cost[i][j] = calc_cost(i,j);
		}
		

    }

	/*
	for(int i=0;i< domainCount;i++){
		for(int j=0;j<nodeCount;j++){
			cout<<"domain"<<i<<":node"<<j<<"-C:"<<C[i][j]<<endl;
			cout<<"domain"<<i<<":node"<<j<<"-CPU:"<<CPU[i][j]<<endl;
		}
	}
	*/

	//getMissRate();

	cout<<"finish to getDomainsInfo"<<endl;
	return SUCCESS;
}

/*
 * get MissRate of each domain
 * Jinrui
 * jinrui@jinrui.name
 * 2013-08-24
 */
int SysInfo::getMissRate(){

	//--passive-domains=1,2,3 --passive-images=/boot/vmlinux-3.0.13-0.27-xen,/boot/vmlinux-3.0.13-0.27-xen,/boot/vmlinux-3.0.13-0.27-xen
	char command[1000] = "opcontrol --start-daemon --event=LLC_MISSES:6000:0x41:1:1 --event=LLC_REFS:6000:0x4f:1:1 --xen=/boot/xen-syms-4.1.2_14-0.5.5 --vmlinux=/boot/vmlinux-3.0.13-0.27-xen --active-domains=0 --session-dir=/root/op_samples/ ";
	if(domainCount>0){
		strcat(command,"--passive-domains=");
	}
	for(int i=0;i<domainCount;i++){
		if(i!=domainCount-1){
			sprintf(command, "%s%d,", command, dom_active_id_list[i]);
		}
		else{
			sprintf(command, "%s%d", command, dom_active_id_list[i]);
		}
	}
	if(domainCount>0){
		strcat(command,"--passive-images=");
	}
	for(int i=0;i<domainCount;i++){
		if(i!=domainCount-1){
			strcat(command,"/boot/vmlinux-3.0.13-0.27-xen,");
		}
		else{
			strcat(command,"/boot/vmlinux-3.0.13-0.27-xen");
		}
	}


	if(system(command) < 0){
	 	perror("failed to set opcontrol");
	 	return ERROR;
	 }

	char command2[MAX_CHARS_ONE_LINE] = "opcontrol --start";
	if(system(command2) < 0){
	 	perror("failed to start opcontrol");
	 	return ERROR;
	 }

	this_thread::sleep_for(chrono::seconds(20));

	char command3[MAX_CHARS_ONE_LINE] = "opcontrol --stop;opreport --session-dir=/root/op_samples > result";
	if(system(command3) < 0){
	 	perror("failed to stop opcontrol");
	 	return ERROR;
	 }
	
	
	char cmd[MAX_CHARS_ONE_LINE] = "cat result | grep -E 'domain[0-9]+-[a-z]+' > result2";
	 if(system(cmd) < 0){
	 	perror("failed to grep domains' miss rate");
	 	return ERROR;
	 }
	 //Domains' infomation are in file result2, example as follows:
	 /*
     538 12.7973     11947 10.1256 domain2-apps
     403  9.5861      8732  7.4008 domain2-modules
     351  8.3492     12733 10.7918 domain13-modules
     260  6.1846     16169 13.7039 domain13-apps
     229  5.4472      9808  8.3127 domain1-apps
     211  5.0190      7394  6.2667 domain1-modules
      25  0.5947      3862  3.2732 domain3-xen
      13  0.3092      2697  2.2858 domain2-xen
       9  0.2141      2630  2.2290 domain1-xen
       2  0.0476        65  0.0551 domain3-xen-unknown
       0       0        47  0.0398 domain1-xen-unknown
       0       0        60  0.0509 domain2-xen-unknown
	  */
	

	 //get domain num from result file
	 char cmd2[MAX_CHARS_ONE_LINE] = "grep -E 'domain[0-9]+' result2 -o | grep -E '[0-9]+' -o > result3";
	 if(system(cmd2) < 0){
	 	perror("failed to grep domains' number");
	 	return ERROR;
	 }	 
	

	//open file result2
	FILE *file_p;
	if((file_p = fopen("result2", "r")) == NULL){
		perror("failed to open file 'result2'");
		return ERROR;
	}
	
	//open file result3
	FILE *file_n;
	if((file_n = fopen("result3","r")) == NULL){
		perror("failed to open file 'result3'");
		return ERROR;
	}

	double sample1;
	double sample2;
	double uselessData;
	//char domainName[80];
	int domainNum;

	double domainMissSum[MAX_DOMS_NUM];
	double domainSampleSum[MAX_DOMS_NUM];
	
	for(int i=0;i<domainCount;i++){
		domainMissSum[i] = 0;
		domainSampleSum[i] = 0;
	}

	while(fscanf(file_p,"%lf",&sample1)>0){
		fscanf(file_p,"%lf",&uselessData);
		fscanf(file_p,"%lf",&sample2);
		fscanf(file_p,"%lf",&uselessData);
		//fscanf(file_p,"%s",domainName);
		//printf("%f:%f\n",sample1,sample2);
		
		
		fscanf(file_n,"%d",&domainNum);
		domainMissSum[domainNum] += sample1;
		domainSampleSum[domainNum] += sample2;
	}

	for(int i=0;i<MAX_DOMS_NUM;i++){
		MissRate[i] = 0;
		if(domainSampleSum[i]!=0){
			MissRate[i] = domainMissSum[i] / domainSampleSum[i];
			//cout << "MissRate of domain" << i << ":" << MissRate[i] << endl;
		}
		else{
			MissRate[i] = 0;
		}
	}
	fclose(file_p);
	fclose(file_n);
	return SUCCESS;
}
