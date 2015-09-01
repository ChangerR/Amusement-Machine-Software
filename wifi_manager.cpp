#include "wifi_manager.h"

#ifdef SLSERVER_LINUX
#include "util.h"
#include <string.h>

wifi_manager::wifi_manager() {
	_ctrl = NULL;
	_event_ctrl = NULL;
	_is_running = false;
	_buffer = NULL;
}

wifi_manager::~wifi_manager() {

	_is_running = false;

	pthread_join(_thread,NULL);

	clear();

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
		
		if(!_ctrl) {
			printf("wifi open %s failed\n",path);
			break;
		}
		_event_ctrl = wpa_ctrl_open(path);
		
		if(!_event_ctrl) {
			printf("wifi open %s failed\n",path);
			break;
		}
		if(wpa_ctrl_attach(_event_ctrl))
			break;
		
		_is_running = true;
		
		if (0 != pthread_create(&_thread, NULL, wifi_manager::_wifi_recv_thread, (void*)this)) {
			printf("error when create pthread in wifi_manager,%d\n", errno);
			break;
		}
		
		ret = true;
		
	}while(0);
	
	return ret;
}

bool wifi_manager::getWifiStatus(wifi_status* _status) {
	
	bool ret = false;
	size_t len = WIFI_BUFFER_LEN - 1;
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
				sl_hwaddr_aton(p_start + 6,_status->bssid);
			}else if(strncmp(p_start,"id=",3) == 0) {
				_status->id = parse_int_dec(p_start + 3);
			}else if(strncmp(p_start,"ssid=",5) == 0) {
				strncpy(_status->ssid,p_start + 5 ,128);
			}else if(strncmp(p_start,"ip_address=",11) == 0) {
				strncpy(_status->ip_address,p_start + 11,16);
			}
			
			p_start = p_end + 1;
		}
	
		ret = true;
	}while(0);
	
	return ret;
}
	
bool wifi_manager::getAvaiableWifi(list<wifi_scan*> *_list) {
	bool ret = false;
	size_t len = WIFI_BUFFER_LEN - 1;
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
		//	printf("--line:%s\n",p_start);
			p_end = sl_find_first_char(p_start,'\n');
			*p_end = 0;
			
			if(strncmp(p_start,"bssid",5) != 0) 
			{
				wifi_scan* _scan = new wifi_scan;

				memset(_scan,0,sizeof(wifi_scan));

				char _buf[256];
				char *p1 = p_start,*p2 = _buf;
				
				while(*p1 != '\t')
					*p2++ = *p1++;
				
				*p2 = 0;
				p1 ++;
				
				sl_hwaddr_aton(_buf,_scan->bssid);
				
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
				
				_list->push_back(_scan);
			}
			
			p_start = p_end + 1;
		}
		ret = true;	
	}while(0);
	
	return ret;
}

bool wifi_manager::connectWifi(const char* ssid) {
	list<wifi_list*> _configList;
	bool ret = false;
	wifi_list* _wifi = NULL;
	do {
		if(listConfigedWifi(&_configList) == false)
			break;
		
		for(list<wifi_list*>::node* _p_wifi = _configList.begin(); _p_wifi != _configList.end();
					_p_wifi = _p_wifi->next) {
			if(strcmp(_p_wifi->element->ssid,ssid) == 0) {
				_wifi = _p_wifi->element;
				break;
			}			
		}
		
		if(_wifi) {
			ret = connectWifi(_wifi->id);
		}

		clearWifiList(&_configList);
	}while(0);
	
	return ret;
}

bool wifi_manager::connectWifi(int id) {
	bool ret = false;
	size_t len = WIFI_BUFFER_LEN - 1;
	char buf[64] = {0};
	
	do {
		sprintf(buf,"SELECT_NETWORK %d",id);
		
		if(wpa_ctrl_request(_ctrl,buf,strlen(buf),_buffer,&len,NULL))
			break;
		
		//printf("%s\n",buf);
		ret = true;
	}while(0);
	
	return ret;
}

bool wifi_manager::listConfigedWifi(list<wifi_list*> *_list) {
	bool ret = false;
	size_t len = WIFI_BUFFER_LEN - 1;
	char* p_start,*p_end;
	do {
		if(_list==NULL || _ctrl == NULL)
			break;
		
		if(wpa_ctrl_request(_ctrl,"LIST_NETWORKS",sizeof("LIST_NETWORKS"),_buffer,&len,NULL))
			break;
		
		_buffer[len] = 0;
		p_start = _buffer;
		
		while(p_start < _buffer + len) {
			p_end = sl_find_first_char(p_start,'\n');
			*p_end = 0;
			
			if(strncmp(p_start,"network",7) != 0) 
			{
				wifi_list *_wifi_list = new wifi_list;
				char _buf[256];
				char *p1 = p_start,*p2 = _buf;
				
				while(*p1 != '\t')
					*p2++ = *p1++;
				
				*p2 = 0;
				p1 ++;
				
				_wifi_list->id = parse_int_dec(_buf);
				
				p2 = _buf;
				while(*p1 != '\t')
					*p2++ = *p1++;
				
				*p2 = 0;
				p1 ++;
				
				strncpy(_wifi_list->ssid,_buf,128);
				
				while(*p1 != '\t')p1++;				
				p1++;
				
				p2 = _buf;
				while(*p1 != '\t')
					*p2++ = *p1++;
				
				*p2 = 0;
				p1 ++;
				
				if(strncmp(_buf,"[CURRENT]",strlen("[CURRENT]")) == 0)
					_wifi_list->flag = 1;
				else
					_wifi_list->flag = 0;
				
				_list->push_back(_wifi_list);
			}
			p_start = p_end + 1;
		}

		ret = true;
	}while(0);
	
	return ret;
}

bool wifi_manager::addWifiNetwork(const char* ssid,const char* passwd) {
	bool ret = false;
	size_t len = WIFI_BUFFER_LEN - 1;
	char buf[256] = {0};
	int id;
	char* p;
	do {
		if(wpa_ctrl_request(_ctrl,"ADD_NETWORK",sizeof("ADD_NETWORK"),_buffer,&len,NULL))
			break;
		
		_buffer[len] = 0;
		p = _buffer;

		while(*p != '\n')p++;
		*p = 0;
		id = parse_int_dec(_buffer);
		
		//printf("new network id:%s\n",_buffer);

		sprintf(buf,"SET_NETWORK %d ssid \"%s\"",id,ssid);
		
		if(wpa_ctrl_request(_ctrl,buf,strlen(buf),_buffer,&len,NULL))
			break;
		
		printf("%s\n",buf);

		sprintf(buf,"SET_NETWORK %d psk \"%s\"",id,passwd);
		
		if(wpa_ctrl_request(_ctrl,buf,strlen(buf),_buffer,&len,NULL))
			break;
		_buffer[len] = 0;	
		//printf("%s\n%s",buf,_buffer);
		ret = true;
	}while(0);
	
	return ret;
}

bool wifi_manager::save_wificonfig() {
	bool ret = false;
	size_t len = WIFI_BUFFER_LEN - 1;
	
	do {
		if(wpa_ctrl_request(_ctrl,"SAVE_CONFIG",sizeof("SAVE_CONFIG"),_buffer,&len,NULL))
			break;
		
		ret = true;
	}while(0);
	
	return ret;
}

void wifi_manager::onEvent(const char* event,wifi_event_func _func,void* data) {
	
	wifi_event_call* _call;
	_call = new wifi_event_call;
	
	strcpy(_call->event,event);
	_call->_func = _func;
	_call->data = data;
	
	_call_list.push_back(_call);
}

void* wifi_manager::_wifi_recv_thread(void* data) {
	wifi_manager* _manager = (wifi_manager*)data;
	int ret;
	char buf[256];
	size_t len;
	while(_manager->_is_running) {
		ret = wpa_ctrl_pending(_manager->_event_ctrl);
		
		if(ret == 1) {
			len = 256;
			
			if(!wpa_ctrl_recv(_manager->_event_ctrl,buf,&len)) {
				buf[len] = 0;
				list<wifi_event_call*>::node* pcall = NULL;
				//printf("***%s\n",buf);	
				for(list<wifi_event_call*>::node* _c = _manager->_call_list.begin();_c != _manager->_call_list.end();_c = _c->next) {
					if(strncmp(buf+3,_c->element->event,strlen(_c->element->event)) == 0) {
						pcall = _c;
					}
				}
				
				if(pcall) {
					(*pcall->element->_func)(buf[1] - '0',buf + 3 + strlen(pcall->element->event),pcall->element->data);
				}
			}
		}else if(ret == -1)
			break;
		
		usleep(1);
	}
	
	return NULL;
}	

void wifi_manager::clear() {
	
	for(list<wifi_event_call*>::node* _p = _call_list.begin();_p != _call_list.end(); _p = _p->next) {
		delete _p->element;
	}
	_call_list.clear();
}
#endif
