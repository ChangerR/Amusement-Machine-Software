#ifndef __SL_HARDWARE__H
#define __SL_HARDWARE__H
#include "serial.h"
#include "arduino_cmd.h"
#include <pthread.h>

class hardware {
public:
	hardware();
	virtual ~hardware();
	
	bool init(const char*);
	void on(const char* ,ino_execute,void*);
	int write(const char*);
	
private:
	Serial* serial;
	pthread_t _thread;
	bool running;
	pthread_mutex_t lock;
	static void* recv_thread(void* data);
};

#endif