#ifndef __SLSERILA__H
#define __SLSERILA__H
#include "slconfig.h"
#define MAX_SERIALBUFFER_SIZE 4096*4
enum BUADRATE {
	_B9600,_B115200,
};

class Serial {
public:
	Serial(const char* com);
	virtual ~Serial();
	
	bool begin(BUADRATE b);
	void close();
	int write(const char* buf,int len);
	int read(char* buf,int len);
	
	int available();
	int readline(char* buf);
	int print(int n);
	int print(float f);
	int print(const char* s,...);
	int println(const char* s,...);
	
	bool touchForCDCReset();
private:
	int read();
	void* _user;
	char _buffer[MAX_SERIALBUFFER_SIZE];
	int   _wpos;
	char  _port[32];
	bool running;
#ifdef SLSERVER_LINUX
	int _serial_fd;
#endif
#ifdef SLSERVER_WIN32
	OVERLAPPED* overlapped_read;
	OVERLAPPED* overlapped_write;
	HANDLE hEvent_read;
	HANDLE hEvent_write;
#endif
};
#endif
