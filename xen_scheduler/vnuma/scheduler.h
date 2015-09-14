#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__ 5201314

#include"bitmap.h"

typedef struct _sche_vcpu_obj
{
	uint32_t vcpuid;		/* The VCPU ID */
	uint32_t map_len;		/* The length of cpumap in storage. */
	bitmap cpumap;			/* The destination map to migrate to. */
}sche_vcpu_obj;

typedef struct _sche_dom_obj
{
	uint32_t domid;			/* The domain ID */
	uint32_t nr_sche_vcpus;		/* The number of VCPU need to be migrated. */
	sche_vcpu_obj *sche_vcpus;	/* The migrate information about each vcpu. */
}sche_dom_obj;


typedef struct _scheduler scheduler;

typedef struct _sche_ops
{
	int (*get_info) (void *priv);		/* To get some information of machine and domains, can be NULL. */
	int (*pick) (scheduler *sche);		/* To determine what domains to be migrated, must be implemented. */
	int (*migrate) (scheduler *sche);	/* To migrate the domain, must be implemented. */
	int (*destroy) (scheduler *sche);	/* To free the memory of some objectes previously used, can be NULL. */
}sche_ops;

struct _scheduler
{
	char *name;				/* How do we call the scheduler? */
	int trig_time;				/* The interval time between the scheduler twice work in us. */
	uint32_t nr_sche_doms;			/* The number of domain need to be migrated. */
	sche_dom_obj *sche_doms;		/* The migrate information of each domain*. */
	void *priv_data;			/* Use for each scheduler privately. */
	sche_ops ops;				/* You know it before. */
};

#endif
