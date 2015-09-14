//#include "vnuma/numa_scheduler.h"
#include "SysInfo.h"
#include "Scheduler.h"
#include "Vnuma.h"
#include <mutex>
using namespace std;


int main(int argc, char *argv[])
{
	mutex m;
	SysInfo *sysinfo = SysInfo::getInstance(m);
	sysinfo->start();

	Scheduler *xen_scheduler = Scheduler::getInstance(*sysinfo,m);
	xen_scheduler->start();

//	Vnuma *vnuma = Vnuma::getInstance(*sysinfo,m);
//	vnuma->start();

	sysinfo->join();
	xen_scheduler->join();
//	vnuma->join();

	/*
	scheduler numa_sche;
	int trig_time;
	while(1)
	{
		init_numa_scheduler(&numa_sche);
		trig_time = numa_sche.trig_time;
		if(numa_sche.ops.get_info)
			numa_sche.ops.get_info(numa_sche.priv_data);
		numa_sche.ops.pick(&numa_sche);
		numa_sche.ops.migrate(&numa_sche);
		if(numa_sche.ops.destroy)
			numa_sche.ops.destroy(&numa_sche);
		usleep(trig_time);
	}
	*/
	return 0;
}
