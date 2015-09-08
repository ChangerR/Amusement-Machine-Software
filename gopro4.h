#ifndef __GOPRO_HERO4_
#define __GOPRO_HERO4_
#include "slconfig.h"
#include <pthread.h>
#include "list.h"
#include "HttpUrlConnection.h"
#ifdef __linux__
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>
#include "wifi_manager.h"
#endif

#define GOPRO4_UDP_PORT 8554
#define GOPRO4_MAP_PORT 9000
#define GOPRO4_IP "10.5.5.9"
#define GOPRO4_WOL 9

#define GOPRO4_TRANSFER_PORT 1
#define GOPRO4_TRANSFER_STREAM 2

typedef void (*video_handler)(void*);

class gopro4 {
public:
	gopro4(int trans_t = GOPRO4_TRANSFER_STREAM);
	virtual ~gopro4();
	
	bool init();
	
	bool start();
	
#ifdef SLSERVER_LINUX
	bool start2();
	static void onWifiConnected(int level,const char* msg,void* data);
	static void onWifiDisconnected(int level,const char* msg,void* data);

	char* wifi_scan_results(char* p);
#endif
	void stop();
	
	int addClient(int uid,const char* p);
	void removeClient(int uid);
	int runCommand(const char* cmd);
	
	static void* heartbeat(void *);
	static void* transfer(void*);
	bool gopro_wol(const char* ip, unsigned short port);
	static void* transfer_stream(void* data);
	
	bool test_is_work();
	
	void setVideoOn(video_handler _f,void* data);
	void setVideoOff(video_handler _f,void* data);
	
#if 0
	static bool _init_mac;
	static unsigned char _smac[];
#endif

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
	int _trans_type;
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
	static int ffmpeg_init;
#ifdef SLSERVER_LINUX
	wifi_manager* _wifi;
	bool _isWifiConnect;
	char _ssid[128];
	char _psk[128];
	char _wifi_ctrl_iface[128];
#endif	

	video_handler _vd_on_func;
	void* _vd_on_data;
	video_handler _vd_off_func;
	void* _vd_off_data;
};

#endif
