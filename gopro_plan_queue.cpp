 #include "gopro_plan_queue.h"
#include <string.h>
#include <stdlib.h>
#ifdef SLSERVER_LINUX
#include <unistd.h>
#include <errno.h>
#endif
#ifdef SLSERVER_WIN32
#include <windows.h>
#endif

GoproPlanQueue::GoproPlanQueue() {
	_running = false;
	_gopro4 = new gopro4;
}

GoproPlanQueue::~GoproPlanQueue() {
	stop();
	if (_gopro4)
		delete _gopro4;
}

bool GoproPlanQueue::start() {
	
	if(_running)
		return false;
	
	_running = true;
	
	if (_gopro4->init() == false)
	{
		LOGOUT("***ERROR*** gopro init error\n");
		return false;
	}

	if (0 != pthread_create(&_thread, NULL, GoproPlanQueue::_run_queue, (void*)this)) {
		printf("error when create pthread,%d\n", errno);
		return false;
	}
	
	pthread_mutex_init(&_lock,NULL);
	return true;
}
	
void GoproPlanQueue::stop() {
	if(!_running)
		return;
	
	_running = false;
	
	pthread_join(_thread,NULL);
	pthread_mutex_destroy(&_lock);
}
	
int GoproPlanQueue::addPlan(const char* name) {
	_plan* _p = new _plan;
	memset(_p,0,sizeof(_plan));

	if(name)
		strcpy_s(_p->name,name);

	pthread_mutex_lock(&_lock);
	_queue.push_back(_p);
	pthread_mutex_unlock(&_lock);

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
			pthread_mutex_lock(&pointer->_lock);
			pointer->_queue.erase(p);
			pthread_mutex_unlock(&pointer->_lock);
			const char* cp = plan->name;

			if (strcmp(cp, "gopro_power_on") == 0) {

				pointer->_gopro4->gopro_wol(GOPRO4_IP, GOPRO4_WOL);

			}else if (strcmp(cp, "start") == 0) {
#ifdef SLSERVER_LINUX
				pointer->_gopro4->start2();	
#else
				pointer->_gopro4->start();
#endif
				LOGOUT("***INFO*** gopro start\n");
			} else if (strcmp(cp, "stop") == 0) {
				LOGOUT("***INFO*** gopro stop\n");
				pointer->_gopro4->stop();
			} 
#ifdef SLSERVER_LINUX
			else if(strcmp(cp,"wifi_scan") == 0) {

			} else if(strncmp(cp,"connect_wifi(",13) == 0) {
			
			} else if(strcmp(cp,"disconnect_wifi") == 0 ) {
			
			} 
#endif
			else if (pointer->_gopro4->test_is_work())
			{
				pointer->_gopro4->runCommand(cp);
			}
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
