#ifndef __SLSERVER_UTIL__
#define __SLSERVER_UTIL__

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


inline const char* sl_find_first_char(const char* p,char c) {
	while(*p) {
		if(*p == c)
			break;
		p++;
	}
	return p;
}

inline char* sl_find_first_char(char* p,char c) {
	while(*p) {
		if(*p == c)
			break;
		p++;
	}
	return p;
}

inline int sl_hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}


inline int sl_hex2byte(const char *hex)
{
	int a, b;
	a = sl_hex2num(*hex++);
	if (a < 0)
		return -1;
	b = sl_hex2num(*hex++);
	if (b < 0)
		return -1;
	return (a << 4) | b;
}

#define ETH_ALEN 6
inline const char * sl_hwaddr_parse(const char *txt, unsigned char *addr)
{
	size_t i;

	for (i = 0; i < ETH_ALEN; i++) {
		int a;

		a = sl_hex2byte(txt);
		if (a < 0)
			return NULL;
		txt += 2;
		addr[i] = a&0xff;
		if (i < ETH_ALEN - 1 && *txt++ != ':')
			return NULL;
	}
	return txt;
}

inline int sl_hwaddr_aton(const char *txt,unsigned char* addr) {
	return sl_hwaddr_parse(txt, addr) ? 0 : -1;
}

inline char* get_file_from_path(const char* path,char* buf) {
	const char* p1;
	char* p2;

	p2 = buf;
	p1 = path;
	while(*p1)p1++;
	
	while(p1 >= path) {
		if(*p1 == '/')
			break;
		p1--;
	}
	
	if(*p1 == '/')
		p1++;
	
	while(*p1) {
		*p2++ = *p1++;
	}
	*p2 = 0;
	return buf;
}

inline int sl_constrain(int a,int _max,int _min) {
	a = a > _max ? _max : a;
	a = a < _min ? _min : a;
	return a;
}

inline int sl_abs(int a) {
	return a < 0 ? -a : a;
}
#endif
