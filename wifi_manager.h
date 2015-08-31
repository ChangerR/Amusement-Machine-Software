#ifndef __SLSERVER_WIFI_MANAGER__
#define __SLSERVER_WIFI_MANAGER__
#include "slconfig.h"

#ifdef SLSERVER_LINUX
#include <unistd.h>
#include "wpa_ctrl.h"
#include "list.h"
#include <pthread.h>

#define WIFI_BUFFER_LEN 1024
struct wifi_status {
	char bssid[8];
	char ssid[128];
	int id;
	char ip_address[16];
};

struct wifi_scan {
	char bssid[8];
	int frequency;
	int signal_level;
	char flags[128];
	char ssid[128];
};

struct wifi_list {
	int id;
	char ssid[128];
	int flag;
};

typedef void (*wifi_event_func)(const char*,void*);

class wifi_manager {
	
public:

	wifi_manager();
	
	virtual ~wifi_manager();

	bool init(const char* path);
	
	bool getWifiStatus(wifi_status* _status);
	
	bool getAvaiableWifi(list<wifi_scan*> *_list);
	
	bool connectWifi(const char* ssid);
	
	bool connectWifi(int id);
	
	bool listConfigedWifi(list<wifi_list*> *_list);
	
	bool addWifiNetwork(const char* ssid,const char* passwd);
	
	bool save_wificonfig();
	
	void onEvent(const char* event,wifi_event_func _func,void* data);
	
	static void* _wifi_recv_thread(void* data);
	
private:

	struct wifi_event_call {
		char event[128];
		wifi_event_func _func;
		void* data;
	};
	
	pthread_t _thread;
	
	bool _is_running;
	
	wpa_ctrl* _ctrl;
	
	wpa_ctrl* _event_ctrl;
	
	char* _buffer;
	
	list<wifi_event_call*> _call_list;
};

template <class T>
inline void clearWifiList(list<T> *_list) {
	
	if(!_list)
		return;
	
	for(list<T>::node* _scan = _list->begin();_scan != _list->end(); _scan = _scan->next) {
		delete _scan->element;
	}
}

#endif

#endif
