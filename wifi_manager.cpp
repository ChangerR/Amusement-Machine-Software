#include "wifi_manager.h"

#ifdef SLSERVER_LINUX

wifi_manager::wifi_manager() {
	_ctrl = NULL;
	_event_ctrl = NULL;
}

wifi_manager::~wifi_manager() {
	if(_ctrl)
		wpa_ctrl_close(_ctrl);
	
	if(_event_ctrl)
		wpa_ctrl_close(_event_ctrl);
}

bool wifi_manager::init(const char* path) {
	bool ret = false;
	
	do {
		
		_ctrl = wpa_ctrl_open(path);
		
		if(_ctrl)
			break;
		
		_event_ctrl = wpa_ctrl_open(path);
		
		if(_event_ctrl)
			break;
		
	}while(0);
	
	return ret;
}

bool getWifiStatus(wifi_status* _status);
	
	bool getAvaiableWifi(list<wifi_scan*> *_list);
	
	bool connectWifi(const char* ssid);
	
	bool connectWifi(int id);
	
	bool listConfigedWifi(list<wifi_list*> *_list);
	
	bool addWifiNetwork(const char* ssid,const char* passwd);
	
	bool save_wificonfig();
	
	void onEvent(const char* event,wifi_event_call _call,void* data);
	
#endif