#include<sys/time.h>
#include<inttypes.h>
#include<float.h>
#include"numa_scheduler.h"

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#define deal_ret(retval,...) \
do\
{\
	if(retval<0)\
	{\
		fprintf(stderr,"%s:",__FUNCTION__);\
		fprintf(stderr,__VA_ARGS__);\
		ret = retval;\
		goto error;\
	}\
}while(0)

#undef set_xen_guest_handle
#define set_xen_guest_handle(hnd, val)  do { (hnd).p = val; } while (0)
#define SEC_TO_USEC(_s) ((_s) * 1000000UL)
#define NSEC_TO_USEC(_nsec) ((_nsec) / 1000UL)
#define PAGE_TO_MEMKB(pages) ((pages) * 4)

/*curtime minus oldtime in us.*/
#define cal_intv(curtime, oldtime) (SEC_TO_USEC( curtime.tv_sec - oldtime.tv_sec ) + curtime.tv_usec - oldtime.tv_usec)
int init_numa_scheduler(scheduler *sche)
{
	sche_ops ops = {
                        .get_info = numa_sche_get_info,
                        .pick = numa_sche_pick,
                        .migrate = numa_sche_migrate,
                        .destroy = numa_sche_destory,
        };
	numa_sche_priv *npriv=NULL;
	
	sche->name = "numa scheduler";
	sche->trig_time = 1000000;	/* 1 sec*/
	sche->priv_data = malloc(sizeof(numa_sche_priv));
	memset(sche->priv_data, 0, sizeof(numa_sche_priv));
	npriv = (numa_sche_priv *)sche->priv_data;
	npriv->xch = xc_interface_open(0, 0, 0);
	sche->nr_sche_doms = 0;
	sche->sche_doms = (void *)malloc(MAX_DOM_NR * sizeof(sche_dom_obj));
	sche->ops = ops;
	return 0;
}

/*Get the node distribution of each domain memory.*/
static int get_dom_mem_dist(numa_sche_priv *npriv)
{
	xc_interface *xch=npriv->xch;
	char buffer[MAX_BUFFER_LEN], *token = buffer;
	unsigned int count = MAX_BUFFER_LEN;
	int i, j, index = 0,ret = 0, buf_index = 0, node;
	uint32_t domid,mem_total;

	/* xc_readconsolering is call by xl dmesg, set clear = 1 to clear it.*/
	xc_readconsolering(xch, buffer, &count, 1, 1, &index);
	xc_send_debug_keys(xch, "u");

	/* read until end.*/
	while(1)
	{
		ret = xc_readconsolering(xch, &buffer[buf_index], &count, 0, 1, &index);
		if(unlikely(ret < 0))
			return ret;
		if(count == 0)
			break;
		buf_index += count;
	}
	
	token =  strtok(buffer, "\n");

	/* ignore some useless imformation*/
	while(token)
	{
		if(!strcmp(token, "(XEN) Memory location of each domain:"))
		{
			token = strtok(NULL, "\n");
			break;
		}
		token = strtok(NULL, "\n");
	}

	/* read the distribution of each domain memory through xl dmesg output*/
	for(i = 0; i < npriv->nr_doms && token; i++)
	{
		index = i;
		sscanf(token, "(XEN) Domain %u (total: %u):",&domid, &mem_total);
		if(unlikely(npriv->domains[i].domid != domid))
		{
			index = -1;
			for(j = 0; j < npriv->nr_doms; j++)
			{
				if(unlikely(npriv->domains[i].domid == domid))
				{
					index = j;
					break;
				}
			}
			if(unlikely(index == -1))
			{
				fprintf(stderr,"%s:Could not find domain %u.", __FUNCTION__, domid);
				return;
			}
		}
		token = strtok(NULL, "\n");
		for(j = 0; j < npriv->nr_nodes && token; j++)
		{
			sscanf(token, "(XEN)     Node %d: %"PRIu64"", &node, &npriv->domains[i].mem_dstr[j]);
			token = strtok(NULL, "\n");
		}	
	}
	
	return 0;
}

/* Get some infomation about physical machine, such like nodes, cpus ect.*/
static inline int get_static_phy_info(numa_sche_priv *npriv)
{
	xc_interface *xch=npriv->xch;

	DECLARE_HYPERCALL_BUFFER(xc_node_to_memsize_t, memsize);
    	DECLARE_HYPERCALL_BUFFER(xc_node_to_memfree_t, memfree);
    	DECLARE_HYPERCALL_BUFFER(uint32_t, node_dists);

	xc_numainfo_t ninfo;
	xc_physinfo_t physinfo;
	xc_topologyinfo_t tinfo = {0};

	int ret = 0, max_nodes, max_cpus, nr_cpus, nr_vcpus,nr_doms, i, j;

	ret = xc_physinfo(xch, &physinfo);
	deal_ret(ret,"xc_physinfo() failed!\n");

	npriv->max_nodes = max_nodes = physinfo.max_node_id + 1;
	npriv->nr_nodes = physinfo.nr_nodes;
	npriv->max_cpus = max_cpus = physinfo.max_cpu_id + 1;
	npriv->nr_cpus = nr_cpus = physinfo.nr_cpus;

	memsize = xc_hypercall_buffer_alloc(xch, memsize, sizeof(*memsize) * max_nodes);
    	memfree = xc_hypercall_buffer_alloc(xch, memfree, sizeof(*memfree) * max_nodes);
    	node_dists = xc_hypercall_buffer_alloc(xch, node_dists, sizeof(*node_dists) * max_nodes * max_nodes);
	
	set_xen_guest_handle(ninfo.node_to_memsize, memsize);
    	set_xen_guest_handle(ninfo.node_to_memfree, memfree);
    	set_xen_guest_handle(ninfo.node_to_node_distance, node_dists);
    	ninfo.max_node_index = max_nodes - 1;
	
	ret=xc_numainfo(xch, &ninfo);
	deal_ret(ret,"xc_numainfo() failed!\n");

	if (ninfo.max_node_index < max_nodes - 1)
        max_nodes = ninfo.max_node_index + 1;

	npriv->nr_nodes = max_nodes;

	for (i = 0; i < max_nodes; i++) 
	{
		npriv->memfree[i]=memfree[i];
		npriv->memsize[i]=memsize[i];
		for(j = 0; j < max_nodes; j++)
			npriv->node_dists[i * max_nodes + j] = node_dists[i * max_nodes + j];
	}

	for(i = 0; i < MAX_CPU_NR; i++)
		npriv->cpu_to_node[i] = INVALID_NODE;
	
	set_xen_guest_handle(tinfo.cpu_to_core, NULL);
    	set_xen_guest_handle(tinfo.cpu_to_socket, NULL);
   	set_xen_guest_handle(tinfo.cpu_to_node, npriv->cpu_to_node);
    	tinfo.max_cpu_index = MAX_CPU_NR - 1;

    	ret = xc_topologyinfo(xch, &tinfo);
	deal_ret(ret,"xc_topologyinfo() failed!\n");

error:
	xc_hypercall_buffer_free(xch,memsize);
	xc_hypercall_buffer_free(xch,memfree);
	xc_hypercall_buffer_free(xch,node_dists);
	return ret;
	
}

/*Get the information about each domain, and the cpu usage.*/
static inline int get_dynamic_info(numa_sche_priv *npriv)
{
	xc_interface *xch=npriv->xch;
	
	xc_cpuinfo_t oldcinfo[MAX_CPU_NR],curcinfo[MAX_CPU_NR];
	xc_domaininfo_t dom_info[MAX_DOM_NR];
	xc_vcpuinfo_t vcpu_info;
	
	int ret = 0, max_nodes = npriv->max_nodes, max_cpus = npriv->max_cpus;
	int nr_cpus = npriv->nr_cpus, nr_vcpus, nr_doms = npriv->nr_doms, i, j, node;
	double idle_intv, sys_intv, cpu_intv;
	unsigned cpu_cnt[MAX_NODE_NR]= {0};
	uint64_t cputime[MAX_DOM_NR];

	struct timeval oldtime, curtime, cpu_oldtime, cpu_curtime;
	
	ret = gettimeofday(&cpu_oldtime, NULL);
        deal_ret(ret, "gettimeofday() failed!\n");

	nr_doms = xc_domain_getinfolist(xch, 0, MAX_DOM_NR, dom_info);
	deal_ret(nr_doms,"xc_domain_getinfolist() failed!\n");
	
	npriv->nr_doms = nr_doms;

	for (i = 0; i < nr_doms; i++)
	{
		npriv->domains[i].domid = dom_info[i].domain;
		npriv->domains[i].nr_vcpus = dom_info[i].nr_online_vcpus;
		nr_vcpus = npriv->domains[i].max_vcpu = dom_info[i].max_vcpu_id + 1;
		npriv->domains[i].cur_mem = PAGE_TO_MEMKB(dom_info[i].tot_pages);
		npriv->domains[i].max_mem = PAGE_TO_MEMKB(dom_info[i].max_pages);
		npriv->domains[i].vcpus = 
			(struct _vcpu_info *)malloc(dom_info[i].nr_online_vcpus * sizeof(vcpu_info));
		npriv->domains[i].mem_dstr = 
			(uint64_t *)malloc(npriv->max_nodes * sizeof(uint64_t));
		memset(npriv->domains[i].vcpus, 0, 
			dom_info[i].nr_online_vcpus * sizeof(vcpu_info));
		memset(npriv->domains[i].mem_dstr, 0, 
			npriv->max_nodes * sizeof(uint64_t));		
		for(j=0; j < nr_vcpus; j++)
		{
			ret = xc_vcpu_getinfo(xch, dom_info[i].domain, j, &vcpu_info);
			if(ret >= 0)
			{
				npriv->domains[i].vcpus[j].vcpuid = j;
				npriv->domains[i].vcpus[j].online = vcpu_info.online;
				npriv->domains[i].vcpus[j].blocked = vcpu_info.blocked;
				npriv->domains[i].vcpus[j].running = vcpu_info.running;
				npriv->domains[i].vcpus[j].cpu = vcpu_info.cpu;
			}
		}
		cputime[i] = dom_info[i].cpu_time;
	}
	
	ret = get_dom_mem_dist(npriv);
	deal_ret(ret, "get_dom_mem_dist() failed!\n");

	ret = gettimeofday(&oldtime, NULL);
	deal_ret(ret, "gettimeofday() failed!\n");

	ret = xc_getcpuinfo(xch, max_cpus, oldcinfo, &nr_cpus);
	deal_ret(ret, "xc_getcpuinfo() failed!\n");

	usleep(WAIT_MS);
	
	ret = gettimeofday(&curtime, NULL);
        deal_ret(ret, "gettimeofday() failed!\n");

        ret = xc_getcpuinfo(xch, max_cpus, curcinfo, &nr_cpus);
        deal_ret(ret, "xc_getcpuinfo() failed!\n");
	
	sys_intv = cal_intv(curtime, oldtime);

	if ((int)sys_intv == 0)
		perror("Not sleep well!\n");

	memset(npriv->node_cpu_usage , 0, sizeof(double) * MAX_NODE_NR);

	for(i = 0; i < max_cpus; i++)
	{
		node = npriv->cpu_to_node[i];
		if(node == INVALID_NODE)
			continue;
		idle_intv = NSEC_TO_USEC(curcinfo[i].idletime - oldcinfo[i].idletime);
		npriv->cpu_usage[i]= 1 - idle_intv / sys_intv;
		npriv->node_cpu_usage[node] += npriv->cpu_usage[i];
		cpu_cnt[node]++;
	}

	for(i = 0; i < max_nodes; i++)
		if(cpu_cnt[node] != 0)
			npriv->node_cpu_usage[node] /= cpu_cnt[node];
	
        ret = gettimeofday(&cpu_curtime, NULL);
        deal_ret(ret, "gettimeofday() failed!\n");
	
	/*mainly to get the cpu time of each domain*/
	nr_doms = xc_domain_getinfolist(xch, 0, MAX_DOM_NR, dom_info);
        deal_ret(nr_doms,"xc_domain_getinfolist() failed!\n");
	
	sys_intv = cal_intv(cpu_curtime, cpu_oldtime);

	for (i = 0; i < nr_doms; i++)
	{
		cpu_intv = NSEC_TO_USEC(dom_info[i].cpu_time - cputime[i]);
		npriv->domains[i].cpu_usage = cpu_intv / sys_intv;
	}

error:
	return ret;
}

int numa_sche_get_info(void *priv)
{	
	numa_sche_priv *npriv=(numa_sche_priv *)priv;
	int ret = 0;

	ret =  get_static_phy_info(npriv);
	deal_ret(ret, "get_static_phy_info() error!\n");
	
	ret = get_dynamic_info(npriv);
	deal_ret(ret, "get_dynamic_info() error!\n");

error:
	return ret;
}

#define dom_in_node(dom,node) ((dom).mem_dstr[node] != 0 )

static uint64_t get_phys_mem_size(numa_sche_priv *npriv)
{
	int i;
	uint64_t mem = 0;
	for(i = 0; i < npriv->max_nodes; i++)
		mem += npriv->memsize[i];
	return mem;
}

/*caculate the cost of migrate dom to node (int)node.*/
static double cal_cost(numa_sche_priv *npriv, dom_info *dom, int node)
{
	int i,max_nodes = npriv->max_nodes;
	double dist = 0;
	
	for(i = 0; i < max_nodes; i++)
		dist += dom->mem_dstr[i * max_nodes + node] * npriv->node_dists[i * max_nodes + node];
	return dist;
}

int numa_sche_pick(scheduler *sche)
{
	numa_sche_priv *npriv=(numa_sche_priv *)sche->priv_data;
	int i, j, ret = 0, max_load_index = -1, min_load_index = -1;
	double load_fac[MAX_NODE_NR] = {0}, cpu_usage, mem_usage, max_load_fac = 0, mem_free_prop;
	double min_load_fac = (1 / (1 - MAX_CPU_USAGE)) * (1 / MIN_MEM_FREE_PROP);
	int node_selected = -1, dom_selected = -1; 
	double vm_weight,max_weight = 0, min_cost = DBL_MAX, cost;
	uint64_t phys_memory = 0; 
	dom_info *dom;
	bitmap nodemap;

	/* to find the node in the heaviest load and the one in lightest load */
	for(i = 0; i < npriv->nr_nodes; i++)
	{

		if(unlikely(npriv->memsize[i] == 0))
			continue;

		if(unlikely(npriv -> node_cpu_usage[i] > MAX_CPU_USAGE))
			cpu_usage = MAX_CPU_USAGE;
		else
			cpu_usage = npriv->node_cpu_usage[i];
		
		mem_free_prop = (double)npriv->memfree[i] / npriv->memsize[i];
		if(unlikely(mem_free_prop < MIN_MEM_FREE_PROP))
			mem_free_prop = MIN_MEM_FREE_PROP;

		load_fac[i] = (1 / (1 - cpu_usage)) * (1 / mem_free_prop);
		
		if(load_fac[i] > max_load_fac)
		{	
			max_load_fac = load_fac[i];
			max_load_index = i;						
		}
		
		if(load_fac[i] < min_load_fac)
		{
			min_load_fac = load_fac[i];
			min_load_index = i;
                }
	}
	
	if(unlikely(max_load_index < 0) || unlikely(min_load_index < 0))
		return -1;

	/* to find which domain should be migrated*/
	phys_memory = get_phys_mem_size(npriv);	
	for( i = 0; i < npriv->nr_doms; i++)
	{
		dom = &npriv->domains[i];

		if(dom->domid == 0)
			continue;

		if(dom_in_node(*dom, max_load_index))
		{
			vm_weight = (double)dom->nr_vcpus / npriv->nr_cpus * 1
				 / (1 - (double)dom->cur_mem / phys_memory);
			if(vm_weight > max_weight)
			{
				dom_selected = i;
				max_weight = vm_weight;
			}
		}
	}

	if(unlikely(dom_selected < 0))
		return -1;
	
	node_selected = min_load_index;
	
	/* to determine which node to migrate to*/
	for(i = 0; i < npriv->nr_nodes; i++)
		if(load_fac[i] - min_load_fac < LOAD_FAC_THRESHOLD )
		{
			cost = cal_cost(npriv, &npriv->domains[dom_selected],i);
			if(cost < min_cost)
			{
				min_cost = cost;
				node_selected = i;
			}
		}	

	sche->sche_doms = (sche_dom_obj *) malloc (sizeof(sche_dom_obj));
	sche->nr_sche_doms = 1;
	sche->sche_doms[0].domid = dom_selected;
	dom = &npriv->domains[dom_selected];
	sche->sche_doms[0].nr_sche_vcpus = dom->nr_vcpus;
	sche->sche_doms[0].sche_vcpus = (sche_vcpu_obj*)malloc(dom->nr_vcpus * sizeof(sche_vcpu_obj));
	nodemap = alloc_bitmap(npriv->max_cpus);
	for(i = 0; i < npriv->max_cpus; i++)
		if(npriv->cpu_to_node[i] == node_selected)
			set_bit(nodemap,i);
	for(i = 0; i < dom->nr_vcpus; i++)
	{
		sche->sche_doms[0].sche_vcpus[i].vcpuid = dom->vcpus[i].vcpuid;
		sche->sche_doms[0].sche_vcpus[i].map_len = cal_bitmap_len(npriv->max_cpus);
		sche->sche_doms[0].sche_vcpus[i].cpumap = alloc_bitmap(cal_bitmap_len(npriv->max_cpus)); 
		bitmap_copy(sche->sche_doms[0].sche_vcpus[i].cpumap, nodemap, cal_bitmap_len(npriv->max_cpus));
	}
	
	free(nodemap);
		
} 

int numa_sche_migrate(scheduler *sche)
{
	numa_sche_priv *npriv=(numa_sche_priv *)sche->priv_data;
	xc_interface *xch=npriv->xch;

	int i,j;
	for(i = 0; i < sche->nr_sche_doms; i++)
		for(j = 0; j < sche->sche_doms[i].nr_sche_vcpus; j++)
		{
			xc_vcpu_setaffinity(xch, sche->sche_doms[i].domid, 
				sche->sche_doms[i].sche_vcpus[j].vcpuid,
				sche->sche_doms[i].sche_vcpus[j].cpumap);
		}
	return 0;			
}

#define sfree(x) do{if(!x)free(x);}while(0)

int numa_sche_destory(scheduler *sche)
{
	int i,j;
	numa_sche_priv *npriv=(numa_sche_priv *)sche->priv_data;
	
	xc_interface_close(npriv->xch);

	for(i = 0; i < npriv->nr_doms; i++)
	{
		sfree(npriv->domains[i].vcpus);
		sfree(npriv->domains[i].mem_dstr);
	}

	sfree(npriv);
	
	for(i = 0; i < sche->nr_sche_doms; i++)
	{
		for(j = 0; j < sche->sche_doms[i].nr_sche_vcpus; j++)
			sfree(sche->sche_doms[i].sche_vcpus[j].cpumap);
		sfree(sche->sche_doms[i].sche_vcpus);
	}
	
	sfree(sche->sche_doms);
}
