#ifndef __SLSERVER_CONFIGS_
#define __SLSERVER_CONFIGS_
#include "slconfig.h"
#include <string.h>
#include "list.h"
#include "util.h"

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
		char name[64];
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