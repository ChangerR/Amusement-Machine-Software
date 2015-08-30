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


inline char* sl_find_first_char(const char* p,char c) {
	while(*p) {
		if(*p == c)
			break;
		p++;
	}
	return p;
}


#endif