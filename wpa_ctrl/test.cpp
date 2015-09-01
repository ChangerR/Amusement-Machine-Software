#include <unistd.h>
#include "wifi_manager.h"
#include <stdio.h>

void print_bssid(unsigned char* s_mac) {
	printf("mac addr:%.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",s_mac[0],s_mac[1],s_mac[2],s_mac[3],s_mac[4],s_mac[5]);
}

void print_wifiscan(wifi_scan* _scan) {
	printf("-----------\n");
	print_bssid(_scan->bssid);
	printf("frequency:%d\n",_scan->frequency);
	printf("signal level:%d\n",_scan->signal_level);
	printf("flags:%s\n",_scan->flags);
	printf("ssid:%s\n",_scan->ssid);
}

void print_wifilist(wifi_list* _list) {
	printf("================\n");
	printf("network id:%d\n",_list->id);
	printf("ssid:%s\n",_list->ssid);
	printf("flag:%d\n",_list->flag);
}

void connected_wifi(int a,const char* p,void* d) {
	printf("!!!log level:%d\n",a);
	printf("!!!log:%s\n",p);
}

int main(int args,char** argv) {
	wifi_manager* _wifi = new wifi_manager;
	do {
		if(_wifi->init("/var/run/wpa_supplicant/wlan0") == false) {
			printf("Open wifi interface error\n");
			break;
		}
		_wifi->onEvent(WPA_EVENT_CONNECTED,connected_wifi,NULL);

		wifi_status _status = {0};

		if(_wifi->getWifiStatus(&_status) == false) {
			printf("getWifiStatus failed\n");
			break;
		}

		print_bssid(_status.bssid);
		printf("ssid:%s\n",_status.ssid);
		printf("wifi id:%d\n",_status.id);
		printf("ip_address:%s\n",_status.ip_address);
		
		list<wifi_scan*> _scan_list;
		
		if(_wifi->getAvaiableWifi(&_scan_list) == false) {
			printf("getWifiStatus failed\n");
			break;
		}
		
		for(list<wifi_scan*>::node* _p = _scan_list.begin();_p != _scan_list.end();_p = _p->next) {
			print_wifiscan(_p->element);
			delete _p->element;
		}

		if(_wifi->addWifiNetwork("gopro12345","12345678") == false) {
			printf("addWifiNetwork failed\n");
			break;
		}

		if(_wifi->connectWifi("gopro12345") == false) {
			printf("connectWifi failed\n");
			break;
		}

		list<wifi_list*> _configlist;

		if(_wifi->listConfigedWifi(&_configlist) == false) {
			printf("listConfigedWifi failed\n");
			break;
		}
		
		for(list<wifi_list*>::node* _pl = _configlist.begin();_pl != _configlist.end();_pl = _pl->next) {
			print_wifilist(_pl->element);
			delete _pl->element;
		}

		sleep(10);
	}while(0);

	delete _wifi;

	return 0;
}
