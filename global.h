#ifndef __SLSERVER_GLOBAL
#define __SLSERVER_GLOBAL

#include "slconfig.h"
#include "ServerConfig.h"

#ifdef SLSERVER_LINUX
#include <pthread.h>
#endif

struct SlGlobal {
	bool is_stream_running;
#ifdef SLSERVER_LINUX
	pthread_mutex_t frame_lock;
#elif defined(SLSERVER_WIN32)
	CRITICAL_SECTION frame_lock;
#endif
	unsigned char* frame;
	int frame_size;
	int frame_alloc_size;
	long frame_count;
	ServerConfig* pConfig;
#ifdef SLSERVER_LINUX
	bool global_running;
#endif
};

extern SlGlobal slglobal;

#ifdef SLSERVER_WIN32
#define INIT_GLOBAL_FRAME_LOCK InitializeCriticalSection(&slglobal.frame_lock)
#define DESTROY_GLOBAL_FRAME_LOCK DeleteCriticalSection(&slglobal.frame_lock)
#define LOCK_GLOBAL_FRAME_LOCK EnterCriticalSection(&slglobal.frame_lock)
#define UNLOCK_GLOBAL_FRAME_LOCK LeaveCriticalSection(&slglobal.frame_lock)
#endif
#endif