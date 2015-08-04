#ifndef __SLSERVER_CONFIGS_
#define __SLSERVER_CONFIGS_
#include "slconfig.h"
#include <string.h>
#include "list.h"

inline int parse_int_dec(const char* p) {
	bool isNeg = false;
	int ret = 0;
	if(*p == '-') {
		isNeg = true;
		p++;
	}
	
	while(*p) {
		if(*p < '0' || *p > '9') {
			ret = 0;
			break;
		}
		ret *= 10;
		ret += *p - '0';
		p++;
	}
	if (isNeg)
		ret *= -1;
	return ret;
}

inline int parse_int_hex(const char* p) {
	bool isNeg = false;
	int ret = 0;
	if(*p == '-') {
		isNeg = true;
		p++;
	}
	if(p[0] != '0' || p[1] != 'x')
		return 0;
	
	while(*p) {
		if((*p >= '0'&&*p <= '9')) {
			ret *= 16;
			ret += *p - '0';
		}else if(*p >= 'A'&&*p <= 'F') {
			ret *= 16;
			ret += *p - 'A' + 10;
		}else if(*p >= 'a'&&*p <= 'f') {
			ret *= 16;
			ret += *p - 'a' + 10;
		}else{
			ret = 0;
			break;
		}	
		p++;
	}
	if (isNeg)
		ret *= -1;
	return ret;
}


inline float parse_float(const char* p) {
	bool isNeg = false;
	float ret = 0,l = 1.f;
	if(*p == '-') {
		isNeg = true;
		p++;
	}
	while(*p&&*p != '.') {
		if(*p < '0' || *p > '9') {
			return 0.f;
		}
		ret *= 10;
		ret += *p - '0';
		p++;
	}
	
	if(*p == '.') {
		p++;
	}
	
	while(*p) {
		if(*p < '0' || *p > '9') {
			return 0.f;
		}
		l *= 0.1f;
		ret += (*p - '0')*l;
		p++;
	}
	
	if (isNeg)
		ret *= -1;
	return ret;
}


class ServerConfig {
public:
	ServerConfig();
	virtual ~ServerConfig();
	
	bool init(const char* p);
	bool parseData();
	bool sync();
	
	bool getBoolean(const char*,bool*);
	bool  getInt(const char*,int*);
	bool getIntHex(const char*,int*);
	bool getString(const char*,char*);
	bool getFloat(const char*,float*);
	bool getMacAddress(const char*, unsigned char*);

	void setBoolean(const char*,bool);
	void setInt(const char*,int);
	void setIntHex(const char*,int);
	void setString(const char*,const char*);
	void setFloat(const char*,float);

	void debug();
private:
	class _CONFIG {
	public:
		_CONFIG() {
			*name = 0;
			*value = 0;
		}
		_CONFIG(const _CONFIG& c) {
			strcpy_s(name,c.name);
			strcpy_s(value,c.value);
		}
		char name[32];
		char value[64];
	};

	list<_CONFIG> _configs;
	
	_CONFIG* find(const char*);
	
	char _filename[256];
	bool _inited;
	static char default_config[];
	char* _buffer;
};

#endif