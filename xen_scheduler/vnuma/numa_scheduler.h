#ifndef __NUMA_SCHEDULER_H__
#define __NUMA_SCHEDULER_H__ 5201314

#include <xenctrl.h>
#include "scheduler.h"

#define MAX_DOM_NR 1024
#define MAX_CPU_NR 256
#define MAX_VCPU_NR MAX_CPU_NR
#define MAX_NODE_NR 64

#define INVALID_NODE -1

#define MAX_BUFFER_LEN 16384
#define MAX_CPU_USAGE 0.9
#define MIN_MEM_FREE_PROP 0.1
#define LOAD_FAC_THRESHOLD 0.1

#define WAIT_MS 30000

typedef struct _vcpu_info 
{
	uint32_t vcpuid;		/* VCPU ID */
	uint8_t  online;                /* currently online (not hotplugged)? */
	uint8_t  blocked;               /* blocked waiting for an event? */
	uint8_t  running;               /* currently scheduled on its CPU? */
	uint32_t cpu;			/* Run in cpu # */
}vcpu_info;

typedef struct _dom_info
{
	uint32_t domid;			/* domain ID */
	uint32_t nr_vcpus;		/* # vcpus in this domain */
	uint32_t max_vcpu;		/* Largest possible VCPU ID plus 1 on this domain */
	uint64_t cur_mem;		/* current memory in KB */
	uint64_t max_mem;		/* maximun memory in KB */
	double cpu_usage;		/* CPU usage in this domain */
	uint64_t *mem_dstr;		/* memory destribute in each node in KB */
	vcpu_info *vcpus;		/* vcpu information */
}dom_info;

typedef struct _numa_sche_priv
{
	xc_interface *xch;
	int max_cpus; 	/* Largest possible CPU ID plus 1 on this host */
	int nr_cpus; 	/* # CPUs currently online */
	int max_nodes;	/* Largest possible node ID plus 1 on this host */
	int nr_nodes;	/* # nodes currently online */
	int nr_doms;    /* # domains currently online */
	unsigned long long memfree[MAX_NODE_NR];	/* Free memory in each node in MB */
	unsigned long long memsize[MAX_NODE_NR];	/* Total memory in each node in MB */
	double cpu_usage[MAX_CPU_NR];			/* Each cpu usage */
	double node_cpu_usage[MAX_NODE_NR];		/* Cpus average usage in each node */
	uint32_t cpu_to_node[MAX_CPU_NR];		/* Cpumap of each node */
	uint32_t node_dists[MAX_NODE_NR*MAX_NODE_NR];	/* Distance between nodes */
	dom_info domains[MAX_DOM_NR];			/* Information about domains */
}numa_sche_priv;

int init_numa_scheduler(scheduler *sche);
int numa_sche_get_info(void *priv);
int numa_sche_pick(scheduler *sche);
int numa_sche_migrate(scheduler *sche);
int numa_sche_destory(scheduler *sche);

#endif
