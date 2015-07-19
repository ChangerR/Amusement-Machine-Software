#include "hardware.h"
#include "arduino_cmd.h"
#include <pthread.h>
#include "slrov.h"
#include <errno.h>
#ifdef SLSERVER_WIN32
#include <windows.h>
#endif
#include <string.h>
#ifdef SLSERVER_LINUX
#include <unistd.h>
#endif

hardware::hardware() {
	serial = NULL;
}

hardware::~hardware() {
	running = false;
	if(serial)
		serial->close();
	pthread_join(_thread, NULL);
	pthread_mutex_destroy(&lock);
	if(serial)
		delete serial;
}

bool hardware::init(const char* p) {
	serial = new Serial(p);
	running = true;
	if(!serial->touchForCDCReset()||serial->begin(_B115200) == false) {
		printf("***ERROR*** init harware\n");
		return false;
	}

	if (0 != pthread_create(&_thread, NULL, hardware::recv_thread, (void*)this)) {
		printf("error when create pthread,%d\n", errno);
		return false;
	}
	pthread_mutex_init(&lock,NULL);
	return true;
}

void hardware::on(const char* p,ino_execute exec,void* user) {
	arduino_cmd::add_command(p,exec,user);
}

int hardware::write(const char* p) {
	int len = strlen(p);
	pthread_mutex_lock(&lock);
	if (-1 == serial->write(p, len)) {
		pthread_mutex_unlock(&lock);
		return -1;
	}
	pthread_mutex_unlock(&lock);
	
	return len;
}

//#ifdef SLSERVER_WIN32
void* __cdecl hardware::recv_thread(void* data) {
//#else
//void* hardware::recv_thread(void* data) {
//#endif
	hardware* t = (hardware*)data;
	char buf[128];
	
	while(t->running) {
		
		if(t->serial->readline(buf) != -1 /*&& arduino_cmd::parse_command(buf)*/) {
			if(strncmp(buf,"ms5803",6)) {
				LOGOUT("***INFO*** SERIAL:%s\n",buf);
			}
			//LOGOUT("***INFO*** SERIAL:%s\n",buf);
			//arduino_cmd::execute();
		}
#ifdef SLSERVER_WIN32
		Sleep(1);
#elif defined(SLSERVER_LINUX)
	//	sched_yield();
		usleep(1);
#endif
	}
	LOGOUT("***INFO*** Hardware Return OK\n");
	return NULL;
}
