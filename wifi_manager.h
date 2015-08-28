#ifndef __SLSERVER_WIFI_MANAGER__
#define __SLSERVER_WIFI_MANAGER__
#include "slconfig.h"

#ifdef SLSERVER_LINUX
#include <unistd.h>
#include "wpa_ctrl.h"
#include "list.h"

struct wifi_status {
	char bssid[8];
	char ssid[256];
	int id;
	char ip_address[16];
};

struct wifi_scan {
	char bssid[8];
	int frequency;
	int signal;
	int level;
	int flags;
	char ssid[128];
};

struct wifi_list {
	int id;
	char ssid[128];
	int flag;
};

typedef void (*wifi_event_call)(const char*,void*);

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
	
	void onEvent(const char* event,wifi_event_call _call,void* data);
	
private:
	wpa_ctrl* _ctrl;
	
	wpa_ctrl* _event_ctrl;
	
};
#endif

#endif
