#ifndef __GOPRO_HERO4_
#define __GOPRO_HERO4_
#include "slconfig.h"
#include <pthread.h>
#include "list.h"
#include "HttpUrlConnection.h"
#ifdef SLSERVER_WIN32
#include <winsock2.h>
#endif
#ifdef __linux__
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h>
#endif

#define GOPRO4_UDP_PORT 8554
#define GOPRO4_MAP_PORT 9000
#define GOPRO4_IP "10.5.5.9"
#define GOPRO4_WOL 9

class gopro4 {
public:
	gopro4();
	virtual ~gopro4();
	
	bool init();
	bool start();
	void stop();
	
	int addClient(int uid,const char* p);
	void removeClient(int uid);
	int runCommand(const char* cmd);
	
	static void* heartbeat(void *);
	static void* transfer(void*);
	static bool gopro_wol(const char* ip, unsigned short port);

	bool test_is_work();

	static bool _init_mac;
	static unsigned char _smac[];
private:
	SOCKADDR_IN _heartbeat_addr;
	int _heart_len;
	char _beat[256];
	int _beat_len;
	
	SOCKET _recv;
	SOCKET _send;
	SOCKET _heartbeat_sender;
	
	pthread_t _beat_thread;
	pthread_cond_t _beat_int_cond;
	pthread_mutex_t _beat_int_mutex;
	pthread_mutex_t _client_mutex;
	bool _beat_running;
	bool _is_start;
	bool _is_working;

	class Client {
	public:
		Client();
		Client(int,const char*);
		int uid;
		SOCKADDR_IN sock;
	};	
	list<Client> _clients;
	bool _trans_running;
	pthread_t _trans_thread;
	char* _recv_buffer;
	HttpUrlConnection _conn;

};

#endif
