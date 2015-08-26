#include "slconfig.h"

#ifdef SLSERVER_WIN32
#include <windows.h>
#include "serial.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>


static DWORD getDCBBuadRate(BUADRATE b) {
	DWORD br = CBR_115200;
	switch(b) {
		case _B9600:
			br = CBR_9600;
			break;
		case _B115200:
			br = CBR_115200;
			break;
	}
	return br;
}	

inline void copy_iner_buffer(char* buf,int from,int to,int len) {
	for(int i = 0; i < len;i++) {
		buf[to + i] = buf[from + i];
	}
}

Serial::Serial(const char* com) {
	strcpy_s(_port,com);

	memset(_buffer,0, MAX_SERIALBUFFER_SIZE);
	
	_wpos = 0;
	_user = 0;
	(overlapped_read) = new OVERLAPPED;
	(overlapped_write) = new OVERLAPPED;
	memset(overlapped_read,0,sizeof(OVERLAPPED));
	memset(overlapped_write,0,sizeof(OVERLAPPED));
	hEvent_read = CreateEvent(NULL, TRUE, FALSE, NULL);
	hEvent_write = CreateEvent(NULL, TRUE, FALSE, NULL);

	running = false;
}

Serial::~Serial() {	
	CloseHandle(hEvent_read);
	CloseHandle(hEvent_write);
	CloseHandle((HANDLE)_user);
	Sleep(100);	
	delete overlapped_read;
	delete overlapped_write;
}

bool Serial::touchForCDCReset() {
	HANDLE hCom;
	hCom = CreateFileA(_port,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	
	if(hCom == INVALID_HANDLE_VALUE) {
		LOGOUT("***ERROR*** open com:%s filed,please check\n",_port);
		return false;
	}
	
	DCB dcb;
	GetCommState(hCom,&dcb);
	
	dcb.BaudRate = CBR_115200;
	dcb.ByteSize = 8; 
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fBinary = 1;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	SetCommState(hCom,&dcb);
	SetupComm(hCom,MAX_SERIALBUFFER_SIZE,MAX_SERIALBUFFER_SIZE);
	
	EscapeCommFunction(hCom,CLRDTR);
	Sleep(200);
	EscapeCommFunction(hCom, SETDTR);

	CloseHandle(hCom);
	
	return true;
}

void Serial::close() {
	running = false;
	//SetEvent(((OVERLAPPED*)overlapped_read)->hEvent);
	//SetEvent(((OVERLAPPED*)overlapped_write)->hEvent);
}

bool Serial::begin(BUADRATE b) {
	HANDLE hCom;
	hCom = CreateFileA(_port,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,NULL);
	
	if(hCom == INVALID_HANDLE_VALUE) {
		LOGOUT("***ERROR*** open com:%s filed,please check\n", _port);
		return false;
	}
	
	DCB dcb;
	GetCommState(hCom,&dcb);
	
	dcb.BaudRate = getDCBBuadRate(b);
	dcb.ByteSize = 8; 
	dcb.Parity = NOPARITY;
	dcb.StopBits = TWOSTOPBITS;
	SetCommState(hCom,&dcb);
	SetupComm(hCom,MAX_SERIALBUFFER_SIZE,MAX_SERIALBUFFER_SIZE);
	
	COMMTIMEOUTS TimeOuts;
	GetCommTimeouts(hCom,&TimeOuts);
	
	TimeOuts.ReadIntervalTimeout = MAXWORD;
	TimeOuts.ReadTotalTimeoutMultiplier = 0;
	TimeOuts.ReadTotalTimeoutConstant = 0;
	
	TimeOuts.WriteTotalTimeoutMultiplier = 40;
	TimeOuts.WriteTotalTimeoutConstant = 50;
	SetCommTimeouts(hCom, &TimeOuts); 
	
	PurgeComm(hCom,PURGE_TXCLEAR|PURGE_RXCLEAR);
	_user = (void*)hCom;
	running = true;
	return true;
}

int Serial::write(const char* buf,int len) {
	HANDLE hCom = (HANDLE)_user;
	DWORD ret = 0;
	if (!running)
		return -1;
	
	memset(overlapped_write, 0, sizeof(OVERLAPPED));
	overlapped_write->hEvent = hEvent_write;

	if(!WriteFile(hCom,buf,len,&ret,overlapped_write)){
		if(GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(overlapped_write->hEvent,INFINITE);
		}else
			return -1;
	}
	
	return ret;
}

int Serial::read() {
	HANDLE hCom = (HANDLE)_user;
	DWORD nRead = MAX_SERIALBUFFER_SIZE - _wpos,nlen;
	COMSTAT ComStat;
	DWORD dwErrorFlags;
	DWORD nError;
	if (!running)
		return -1;

	memset(overlapped_read, 0, sizeof(OVERLAPPED));
	
	if (!ClearCommError(hCom, &dwErrorFlags, &ComStat)) {
		printf("Serial clear comm error :%d\n",GetLastError());
		return -1;
	}
	
	nlen = min(ComStat.cbInQue, nRead);
	if (nlen == 0) {
		return -1;
	}
		
	overlapped_read->hEvent = hEvent_read;

	if(ReadFile(hCom,_buffer + _wpos,nlen,&nRead,overlapped_read) == FALSE) {
		if ((nError = GetLastError()) == ERROR_IO_PENDING) {
			WaitForSingleObject(overlapped_read->hEvent,INFINITE);
		}
		else if(nError == ERROR_INVALID_HANDLE){	
			printf("read Invaild erorr\n");
			return -1;
		}
			
	}
	//((OVERLAPPED*)overlapped_read)->
	_wpos += (nRead == 0 ? nlen : nRead);

	//Sleep(1);
	return _wpos;
}

int Serial::available() {
	
	if(_wpos == 0) {
		read();
	}
	return _wpos;
}

int Serial::read(char* buf,int len) {
	int nRead = -1;
	
	do{
		if(len <= _wpos) {
			memcpy(buf,_buffer,len);
			nRead = len;
			_wpos -= len;
			copy_iner_buffer(_buffer,nRead,0,_wpos);
			break;
		} 
	}while(running&&read() != -1);
	
	return nRead;
}

int Serial::readline(char* buf) {
	int nRead = 0,nRet = 0;
	do {
		for(;nRead < _wpos;nRead++) {
			if(_buffer[nRead] == '\n')
				break;
		}
	}while(running&&_buffer[nRead] != '\n'&&read() != -1);
	
	if(_buffer[nRead] == '\n') {
		memcpy(buf,_buffer,nRead);
		if (buf[nRead - 1] == '\r') {
			buf[nRead - 1] = 0;
			nRet = nRead - 2;
		}
		else {
			buf[nRead] = 0;
			nRet = nRead - 1;
		}
		nRead++;
		_wpos -= nRead;
		copy_iner_buffer(_buffer,nRead,0,_wpos);
		return nRead - 1;
	}
	
	return -1;
}

int Serial::print(int n) {
	char buf[32];
	int nWrite = -1;
	sprintf_s(buf,"%d",n);
	nWrite = strlen(buf);
	return write(buf,nWrite);
}

int Serial::print(float f) {
	char buf[32];
	int nWrite = -1;
	sprintf_s(buf,"%f",f);
	nWrite = strlen(buf);
	return write(buf,nWrite);
}

int Serial::print(const char* s,...) {
	char buf[256];
	int nWrite = -1;
	
	va_list argptr;  
    va_start(argptr, s);  
    vsprintf_s(buf,s, argptr);  
    va_end(argptr);  
	nWrite = strlen(buf);
	return write(buf,nWrite);
}

int Serial::println(const char* s,...) {
	char buf[256];
	int nWrite = -1;
	
	va_list argptr;  
    va_start(argptr, s);  
    vsprintf_s(buf,s, argptr);  
    va_end(argptr);  
	nWrite = strlen(buf);
	buf[nWrite]='\n';
	return write(buf,nWrite+1);
}

#endif
