#ifndef __SLSERVER_CONFIG
#define __SLSERVER_CONFIG

#include "slconfig.h"
#include "ServerConfig.h"
#include <pthread.h>

struct Global {
	bool is_stream_running;
	pthread_mutex_t frame_lock;
	unsigned char* frame;
	int frame_size;
	int frame_alloc_size;
	ServerConfig* pConfig;
};

extern Global global;
#endif