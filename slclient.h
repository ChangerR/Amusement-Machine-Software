#ifndef __SL_CLIENT_H
#define __SL_CLIENT_H
#include "slconfig.h"
#include <pthread.h>

#ifdef SLSERVER_WIN32
#include <winsock2.h>
#endif

#define CLIENT_BUFFER_LEN 1024
#define CLIENT_CREATING 0
#define CLIENT_WORKING 1
#define CLIENT_DEAD 2
#define CLIENT_HTTP 3

#define METHOD_NAME_LEN 8
#define REQUEST_PATH 	128
#define REQUEST_MAGIC 	32

#define BOUNDARY "boundarydonotcross"
#define STD_HEADER 	"Connection: close\r\n" \
					"Server: SlServer/1.0\r\n" \
					"Cache-Control: no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n" \
					"Pragma: no-cache\r\n" \
					"Expires: Mon, 3 Jan 2015 12:34:56 GMT\r\n"

#define LENGTH_OF(x) (sizeof(x)/sizeof(x[0]))

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
	
	int getline_no_remove(char* buf, int max_len);
	int getline(char*,int);
	int write(const char*,int);
	void close();
	
	bool handsank();
	
	int read();
	
	bool http_connect();	
	bool do_http();
	static void* __cdecl http_client_thread(void* arg);
	void send_error(int err,const char* message);
	void send_file(const char* parameter);
	void send_stream();
public:
	SOCKET sock;
	u32 time;
	u32 uid;
	int client_state;
	char ip[16];
	char _buffer[CLIENT_BUFFER_LEN];
	int _wpos;
	static u32 id_counter;
	static char www_folder[];
private:
	
	char _method[METHOD_NAME_LEN];
	char _path[REQUEST_PATH];
	char _magic[REQUEST_MAGIC];
	pthread_t _thread;
};


#endif