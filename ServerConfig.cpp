#include "ServerConfig.h"
#include <string.h>
#include <stdio.h>

char ServerConfig::default_config[] = "#SSLSERVER CONFIG FILE\n" \
								"#BEIGN\n" \
								"#END\n";
								
ServerConfig::ServerConfig() {
	_inited = false;
	_buffer = NULL;
	memset(_filename,0,sizeof(_filename));
}


ServerConfig::~ServerConfig() {
	if(_buffer&&_buffer != default_config)
		delete[] _buffer;
}

bool ServerConfig::init(const char* p) {
	if(p == NULL)
		return false;
	
	strcpy_s(_filename,p);
	FILE* f;
#ifdef SLSERVER_WIN32	
	fopen_s(&f,_filename,"r");
#else
	f = fopen(_filename,"r");
#endif
	if(f == NULL) {
		_buffer = default_config;
	}else{
		int len = 0;
		fseek(f,0,SEEK_END);
		len = ftell(f);
		_buffer = new char[len + 1];
		memset(_buffer,0,len+1);
		fseek(f,0,SEEK_SET);
		fread(_buffer,len,1,f);
		fclose(f);
	}
	
	return true;
}

#define CONFIG_PARSE_BEGIN 0
#define CONFIG_PARSE_END 1
#define CONFIG_PARSE_STRING 2
#define CONFIG_PARSE_ARGUMENT 3
#define CONFIG_PARSE_COMMENT 4
#define CONFIG_PARSE_NAME 5
#define CONFIG_PARSE_ERROR 6

bool ServerConfig::parseData() {
	int parse_state = CONFIG_PARSE_BEGIN;
	char* p = _buffer;
	_CONFIG config;
	int offset = 0;
	if(p == NULL) 
		return false;
	
	while(*p&&parse_state != CONFIG_PARSE_ERROR) {
		
		switch(parse_state) {
		case CONFIG_PARSE_BEGIN:
			if(*p != ' '&& *p != '\t' && *p != '\r' && *p != '\n') {
				if(*p == '#')
					parse_state = CONFIG_PARSE_COMMENT;
				else {
					parse_state = CONFIG_PARSE_NAME;
					config.name[offset++] = *p;
				}
					
			}
			break;
		case CONFIG_PARSE_NAME:
			if(offset >= 32) {
				parse_state = CONFIG_PARSE_ERROR;
				continue;
			}
			if(*p != ' '&&*p != '\t') {
				if(*p == '=') {
					config.name[offset] = 0;
					offset = 0;
					parse_state = CONFIG_PARSE_ARGUMENT;
				}else
					config.name[offset++] = *p;
			}
			break;
		case CONFIG_PARSE_ARGUMENT:
			if(offset >= 64) {
				parse_state = CONFIG_PARSE_ERROR;
				continue;
			}
			if(*p == '\r'||*p == '\n') {
				config.value[offset++] = 0;
				offset = 0;
				parse_state = CONFIG_PARSE_BEGIN;
				_configs.push_back(config);
			} else if(*p == '\"') {
				parse_state = CONFIG_PARSE_STRING;
			} else if(*p != ' '&& *p != '\t') {
				config.value[offset++] = *p;
			}
			break;
		case CONFIG_PARSE_STRING:
			if(offset >= 64) {
				parse_state = CONFIG_PARSE_ERROR;
				continue;
			}
			if(*p == '\"') {
				parse_state = CONFIG_PARSE_ARGUMENT;
			}else 
				config.value[offset++] = *p;
			
			break;
		case CONFIG_PARSE_COMMENT:
			if(*p == '\r' || *p == '\n') {
				parse_state = CONFIG_PARSE_BEGIN;
			}
			break;
		}
		p++;
	}
	
	_inited = true;
	
	if(_buffer&&_buffer != default_config)
		delete[] _buffer;
	_buffer = NULL;
	
	if(parse_state == CONFIG_PARSE_ERROR)
		return false;
	else
		return true;
}

bool ServerConfig::sync() {
	FILE* f = NULL;
	
	if(*_filename == 0)
		return false;
#ifdef SLSERVER_WIN32	
	fopen_s(&f,_filename,"w");
#else
	f = fopen(_filename,"w");
#endif

	if( f == NULL)
		return false;
	
	fprintf(f,"#SSLSERVER CONFIG FILE\n");
	fprintf(f,"#BEIGN\n");
	for(list<_CONFIG>::node *pn = _configs.begin(); pn != _configs.end();pn = pn->next) {
		if (strchr(pn->element.value, ' ') || strchr(pn->element.value, '\t') )
			fprintf(f,"%s = \"%s\"\n",pn->element.name,pn->element.value);
		else
			fprintf(f, "%s = %s\n", pn->element.name, pn->element.value);
	}
	fprintf(f,"#END\n");
	fflush(f);
	fclose(f);
	return true;
}

ServerConfig::_CONFIG* ServerConfig::find(const char* ps) {
	_CONFIG* ret = NULL;
	for(list<_CONFIG>::node *p = _configs.begin(); p != _configs.end();p = p->next) {
		if(!strcmp(p->element.name,ps)) {
			ret = &p->element;
			break;
		}
	}
	return ret;
}

bool ServerConfig::getBoolean(const char* p,bool* data) {
	_CONFIG* ret;
	if((ret = find(p)) == NULL)
		return false;
	
	if(!strcmp("true",ret->value)) {
		*data = true;
		return true;
	}else if(!strcmp("false",ret->value)) {
		*data = false;
		return true;
	}else
		return false;
}

bool  ServerConfig::getInt(const char* p,int* data) {
	_CONFIG* ret;
	
	if((ret = find(p)) == NULL)
		return false;
	
	*data = parse_int_dec(ret->value);
	
	if(*data == 0&&*(ret->value) != '0') {
		return false;
	}
	return true;
	
}

bool ServerConfig::getIntHex(const char* p,int* data) {
	_CONFIG* ret;
	
	if((ret = find(p)) == NULL)
		return false;
	
	*data = parse_int_hex(ret->value);
	
	if(*data == 0&&*(ret->value) != '0') {
		return false;
	}
	return true;
}

bool ServerConfig::getString(const char* p ,char* data) {
	_CONFIG* ret;
	char* ps;
	if((ret = find(p)) == NULL)
		return false;
	
	ps = ret->value;
	
	while (*ps)
	{
		*data++ = *ps++;
	}
	*data = 0;
	return true;
}

bool ServerConfig::getFloat(const char* p,float* data) {
	_CONFIG* ret;
	
	if((ret = find(p)) == NULL)
		return false;
	
	*data = parse_float(ret->value);
	
	if(*data == 0&&*(ret->value) != '0') {
		return false;
	}
	return true;
}

void ServerConfig::setBoolean(const char* p,bool data) {
	_CONFIG* ret;
	_CONFIG d;
	
	if((ret = find(p)) == NULL) {
		strcpy_s(d.name,p);
		strcpy_s(d.value,data?"true":"false");
		_configs.push_back(d);
	}else{
		strcpy_s(ret->value,data?"true":"false");
	}
}

void  ServerConfig::setInt(const char* p,int data) {
	_CONFIG* ret;
	_CONFIG d;
	if((ret = find(p)) == NULL) {
		strcpy_s(d.name,p);
		sprintf_s(d.value,"%d",data);
		_configs.push_back(d);
	}else{
		sprintf_s(ret->value,"%d",data);
	}
}

void ServerConfig::setIntHex(const char* p ,int data) {
	_CONFIG* ret;
	_CONFIG d;
	if((ret = find(p)) == NULL) {
		strcpy_s(d.name,p);
		sprintf_s(d.value,"0x%x",data);
		_configs.push_back(d);
	}else{
		sprintf_s(ret->value,"0x%x",data);
	}
}

void ServerConfig::setString(const char* p,const char* data) {
	_CONFIG* ret;
	_CONFIG d;
	if((ret = find(p)) == NULL) {
		strcpy_s(d.name,p);
		sprintf_s(d.value,"%s",data);
		_configs.push_back(d);
	}else{
		strcpy_s(ret->value,data);
		sprintf_s(d.value, "%s", data);
	}
}

void ServerConfig::setFloat(const char* p ,float data) {
	_CONFIG* ret;
	_CONFIG d;
	if((ret = find(p)) == NULL) {
		strcpy_s(d.name,p);
		sprintf_s(d.value,"%f",data);
		_configs.push_back(d);
	}else{
		sprintf_s(ret->value,"%f",data);
	}
}

void ServerConfig::debug()
{
	LOGOUT("***DEBUG*** Read Config File\n");
	for (list<_CONFIG>::node *p = _configs.begin(); p != _configs.end(); p = p->next) {
		LOGOUT("***DEBUG*** %s=%s\n", p->element.name, p->element.value);
	}
	LOGOUT("***DEBUG*** Config File End\n");
}
