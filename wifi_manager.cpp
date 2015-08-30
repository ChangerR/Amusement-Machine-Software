#include "wifi_manager.h"

#ifdef SLSERVER_LINUX
#include "util.h"
#include "common/common.h"

wifi_manager::wifi_manager() {
	_ctrl = NULL;
	_event_ctrl = NULL;
	
	_buffer = NULL;
}

wifi_manager::~wifi_manager() {
	if(_ctrl)
		wpa_ctrl_close(_ctrl);
	
	if(_event_ctrl)
		wpa_ctrl_close(_event_ctrl);
	
	if(_buffer)
		delete[] _buffer;
}

bool wifi_manager::init(const char* path) {
	bool ret = false;
	
	do {
		if(_buffer == NULL)
			_buffer = new char[WIFI_BUFFER_LEN];
		
		_ctrl = wpa_ctrl_open(path);
		
		if(_ctrl)
			break;
		
		_event_ctrl = wpa_ctrl_open(path);
		
		if(_event_ctrl)
			break;
		
	}while(0);
	
	return ret;
}

bool wifi_manager::getWifiStatus(wifi_status* _status) {
	
	bool ret = false;
	int len = WIFI_BUFFER_LEN - 1;
	char* p_start,*p_end;
	do {
		if(_status==NULL || _ctrl == NULL)
			break;

		memset(_status,0,sizeof(wifi_status));
		
		if(wpa_ctrl_request(_ctrl,"STATUS-VERBOSE",sizeof("STATUS-VERBOSE"),_buffer,&len,NULL))
			break;
		
		_buffer[len] = 0;
		
		p_start = _buffer;
		
		while(p_start < _buffer + len) {
			p_end = sl_find_first_char(p_start,'\n');
			*p_end = 0;
			
			if(strncmp(p_start,"bssid=",6) == 0) {
				hwaddr_aton(p_start + 6,_status->bssid);
			}else if(strncmp(p_start,"id=",3) == 0) {
				_status->id = parse_int_dec(p_start + 3);
			}else if(strncmp(p_start,"ssid=",5) == 0) {
				strncpy(_status->ssid,p_start + 5 ,128);
			}else if(strncmp(p_start,"ip_address",10) == 0) {
				strncpy(_status->ip_address,p_start + 10,16);
			}
			
			p_start = p_end + 1;
		}
		
	}while(0);
	
	return ret;
}
	
bool wifi_manager::getAvaiableWifi(list<wifi_scan*> *_list) {
	bool ret = false;
	int len = WIFI_BUFFER_LEN - 1;
	char* p_start,*p_end;
	do {
		if(_list==NULL || _ctrl == NULL)
			break;
		
		if(wpa_ctrl_request(_ctrl,"SCAN",sizeof("SCAN"),_buffer,&len,NULL))
			break;
		
		len = WIFI_BUFFER_LEN - 1;
		
		if(wpa_ctrl_request(_ctrl,"SCAN_RESULTS",sizeof("SCAN_RESULTS"),_buffer,&len,NULL))
			break;
		
		_buffer[len] = 0;
		
		p_start = _buffer;
		
		while(p_start < _buffer + len) {
			p_end = sl_find_first_char(p_start,'\n');
			*p_end = 0;
			
			if(strncmp(p_start,"bssid",5) == 0) 
				continue;
			else {
				wifi_scan* _scan = new wifi_scan;
				char _buf[256];
				char *p1 = p_start,*p2 = _buf;
				
				while(*p1 != '\t')
					*p2++ = *p1++;
				
				*p2 = 0;
				p1 ++;
				
				hwaddr_aton(_buf,_scan->bssid);
				
				p2 = _buf;
				while(*p1 != '\t')
					*p2++ = *p1++;
				
				*p2 = 0;
				p1 ++;
				
				_scan->frequency = parse_int_dec(_buf);
				
				p2 = _buf;
				while(*p1 != '\t')
					*p2++ = *p1++;
				
				*p2 = 0;
				p1 ++;
				_scan->signal_level = parse_int_dec(_buf);
				
				p2 = _buf;
				while(*p1 != '\t')
					*p2++ = *p1++;
				
				*p2 = 0;
				p1 ++;
				strncpy(_scan->flags,_buf,128);
				
				strncpy(_scan->ssid,p1,128);		
			}
			
			p_start = p_end + 1;
		}
		
	}while(0);
	
	return ret;
}

bool wifi_manager::connectWifi(const char* ssid) {
	
}

bool wifi_manager::connectWifi(int id) {
	
}

bool wifi_manager::listConfigedWifi(list<wifi_list*> *_list) {
	
}

bool wifi_manager::addWifiNetwork(const char* ssid,const char* passwd) {
	
}

bool wifi_manager::save_wificonfig() {
	
}

void wifi_manager::onEvent(const char* event,wifi_event_call _call,void* data) {
	
}
	
#endif