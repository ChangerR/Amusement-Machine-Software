#include "gopro_plan_queue.h"

#ifdef SLSERVER_WIN32
#include <windows.h>
#endif

GoproPlanQueue::GoproPlanQueue() {
	_running = false;
}

GoproPlanQueue::~GoproPlanQueue() {
	stop();
}

bool GoproPlanQueue::start() {
	
	if(_running)
		return false;
	
	_running = true;
	
	if (0 != pthread_create(&_thread, NULL, GoproPlanQueue::_run_queue, (void*)this)) {
		printf("error when create pthread,%d\n", errno);
		return false;
	}
	
	return true;
}
	
void GoproPlanQueue::stop() {
	if(!_running)
		return;
	
	_running = false;
	
	pthread_join(_thread,NULL);
}
	
int GoproPlanQueue::addPlan(PlanFunc func,const char* _arg) {
	_plan* _p = new _plan;
	memset(_p,0,sizeof(_plan));
	
	_p->_func = func;
	
	if(_arg)
		strcpy_s(_p->arg,_arg);
	
	_queue.push_back(_p);
	
	return _queue.getSize();
}

bool GoproPlanQueue::getRunningState() {
	return _running;
}

void* GoproPlanQueue::_run_queue(void*  user) {
	GoproPlanQueue* pointer = (GoproPlanQueue*)user;
	
	while(pointer->getRunningState()) {
		if(!pointer->_queue.empty()) {
			list<_plan*>::node* p = pointer->_queue.begin();
			_plan* plan = p->element;
			pointer->_queue.erase(p);
			plan->_func(plan->arg);
			delete plan;
		}
#ifdef SLSERVER_WIN32
		Sleep(1);
#else
		usleep(1);
#endif
	}
	return NULL;
}