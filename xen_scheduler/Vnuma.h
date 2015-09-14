#ifndef VNUMA_H_
#define VNUMA_H_

#include <mutex>
#include <cmath>
#include "SysInfo.h"
#include "runnable.h"

class Vnuma :public Runnable
{
private:
	SysInfo &sysinfo;													//system info class
	std::mutex *mu;
	void runVnumaScheduler();											//main scheduler function
	double calc_laod(double cpu_usage,double mem_usage);				//calculate the load of domain
	void doMigration(int sourceNode,int targetNode);	//do the migration
public:

	static const int THRESHOLD = 5000;									//if maxload - minload >= threashold, a migration is needed
	Vnuma(SysInfo &sysinfo,std::mutex &m);
	static Vnuma* getInstance(SysInfo &sysinfo,std::mutex &m);
	~Vnuma(void);
	void run();
};

#endif
