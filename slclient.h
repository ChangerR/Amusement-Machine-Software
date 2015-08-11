#ifndef __SL_CLIENT_H
#define __SL_CLIENT_H
#include "slconfig.h"


#ifdef SLSERVER_WIN32
#include <winsock2.h>
#endif

#define CLIENT_BUFFER_LEN 1024
#define CLIENT_CREATING 0
#define CLIENT_WORKING 1
#define CLIENT_DEAD 2

class SlClient {
public:
	SlClient(SOCKET s);
	SlClient() {
		sock = INVALID_SOCKET;
		time = 0;
		client_state = CLIENT_CREATING;
		_wpos = 0;
		uid = id_counter++;
	}
	virtual ~SlClient(){}
	
	int getline_no_remove(char* buf,int max_len)
	int getline(char*,int);
	int write(const char*,int);
	void close();
	
	bool handsank();
	bool http_connect();
	
	int read();
	
public:
	SOCKET sock;
	u32 time;
	u32 uid;
	int client_state;
	char ip[16];
	char _buffer[CLIENT_BUFFER_LEN];
	int _wpos;
	static u32 id_counter;
};


#endif