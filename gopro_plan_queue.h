#ifndef __GOPRO4_PLAN_QUEUE_
#define __GOPRO4_PLAN_QUEUE_
#include "slconfig.h"
#include "list.h"
#include <pthread.h>
#include "gopro4.h"

class SlServer;

class GoproPlanQueue {
public:
	GoproPlanQueue();
	virtual ~GoproPlanQueue();
	
	bool start();
	void stop();
	
	int addPlan(const char* name);
	bool getRunningState();
	
	static void* __cdecl _run_queue(void*);
	
private:
	struct _plan {
		char name[128];
	};
	SlServer* _server;
	list<_plan*> _queue;
	pthread_t _thread;
	bool _running;
	gopro4* _gopro4;
	pthread_mutex_t _lock;
	friend class SlServer;
};

#endif