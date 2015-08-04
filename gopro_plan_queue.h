#ifndef __GOPRO4_PLAN_QUEUE_
#define __GOPRO4_PLAN_QUEUE_
#include "slconfig.h"
#include "list.h"
#include <pthread.h>

typedef int (*PlanFunc)(const char*);

class GoproPlanQueue {
public:
	GoproPlanQueue();
	virtual ~GoproPlanQueue();
	
	bool start();
	void stop();
	
	int addPlan(PlanFunc func,const char* _arg);
	bool getRunningState();
	
	static void* __cdecl _run_queue(void*);
	
private:
	struct _plan {
		PlanFunc _func;
		char arg[128];
	};
	
	list<_plan*> _queue;
	pthread_t _thread;
	bool _running;
};

#endif