#ifndef __SLSERVER_H
#define __SLSERVER_H
#include "slconfig.h"
#ifdef SLSERVER_WIN32
#include <winsock2.h>
#endif
#include "list.h"
#include "slrov.h"
#include <pthread.h>
#include "slclient.h"
#include "ServerConfig.h"
#include "gopro4.h"

class SlServer {
public:
	SlServer(int p,ServerConfig* pconfig);
	virtual ~SlServer();
	
	bool init(const char* p);
	bool start();
	void stop();
	void broadcast(int,const char*);
	void send(int,int,const char*);
	
	static void* __cdecl recv_data(void* data);
	static void* __cdecl watcher(void* data);

	static bool getMacAddr(const char* ip, unsigned char* mac);
public:
	ServerConfig* _config;
private:
	bool running;
	bool watch_running;
	slrov* poilt;
	SOCKET server;
	int port;
	
	list<SlClient*> clients;
	fd_set receiveQueue;
	
	pthread_t _thread;
	pthread_t _timer_thread;
	pthread_cond_t _timer_int_cond;
	pthread_mutex_t _timer_int_mutex;
	pthread_mutex_t _clients_mutex;
	pthread_mutex_t _clients_write_mutex;
	gopro4* _gopro4;
#ifdef SLSERVER_WIN32
	static int socket_init;
	static bool winsock_init();
	static void winsock_close();
#endif
};
#endif