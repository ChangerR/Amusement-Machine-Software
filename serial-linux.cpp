#include "serial.h"
#include <string.h>
#ifdef SLSERVER_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>  
#include <sys/stat.h>   
#include <fcntl.h>     
#include <termios.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/ioctl.h>

static int getDCBBuadRate(BUADRATE b) {
	int br = B115200;
	switch(b) {
		case _B9600:
			br = B9600;
			break;
		case _B115200:
			br = B115200;
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
	strcpy(_port,com);

	memset(_buffer,0,MAX_SERIALBUFFER_SIZE);
	
	_wpos = 0;
	_user = NULL;	
	_serial_fd = -1;
}

Serial::~Serial() {
	close();
}

bool Serial::begin(BUADRATE b) {
	struct termios _option;

	LOGOUT("***INFO*** serial port:%s\n",_port);
	_serial_fd = open(_port,O_RDWR|O_NOCTTY|O_NDELAY);
	
	if(_serial_fd < 0) {
		LOGOUT("***ERROR*** open serial port faile£º%d\n",_serial_fd);
		return false;
	}
	
	tcgetattr(_serial_fd,&_option);

	_option.c_cflag != (CLOCAL|CREAD);
	_option.c_cflag &= ~CSIZE;
	_option.c_cflag &= ~CRTSCTS;
	_option.c_cflag |= CS8;
	_option.c_cflag &= ~CSTOPB;
	_option.c_iflag |= IGNPAR;
	_option.c_oflag &= ~OPOST;
	_option.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	_option.c_cflag &= ~PARENB;   /* Clear parity enable */
	_option.c_iflag &= ~INPCK;     /* Enable parity checking */ 

	cfsetospeed(&_option,getDCBBuadRate(b));
	cfsetispeed(&_option,getDCBBuadRate(b));

	tcflush(_serial_fd,TCIOFLUSH);

	if(tcsetattr(_serial_fd, TCSANOW, &_option) != 0) {
		LOGOUT("***ERROR*** setup serial error\n");
		return false;
	} 
	
	_wpos = 0;
	running = true;
	
	return true;
}

void Serial::close() {
	running  = false;
	::close(_serial_fd);
}

int Serial::write(const char* buf,int len) {
	int _len = 0;
	if(!running)
		return -1;
	
	_len = ::write(_serial_fd,buf,len);
	if(_len == len) {  
        return _len;  
    } else {  
        tcflush(_serial_fd, TCOFLUSH);  
        return -1;  
    }  
}

int Serial::read() {
	int nRead = MAX_SERIALBUFFER_SIZE - _wpos - 1;	
	int len = 0;
	if(!running)
		return -1;
//	printf("serial hardware read enter\n");	
	if((len = ::read(_serial_fd,_buffer + _wpos,nRead)) < 0 ) {
		if(errno != EAGAIN)
			printf("***ERROR*** when read serial occur error errno: %d\n",errno);
		return -1;
	}
//	printf("read serial data len:%d \n",len);	
	_wpos += len;
	//sched_yield();
	_buffer[_wpos] = 0;
	//usleep(1);	
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

int Serial::available() {
	if(_wpos == 0) {
		read();
	}
	return _wpos;

}

int Serial::readline(char* buf) {
	int nRead = 0,nRet = 0;
	
//	printf("readline enter\n");
	do {
		for(;nRead < _wpos;nRead++) {
			if(_buffer[nRead] == '\n')
				break;
		}
	}while(running&&_buffer[nRead] != '\n'&&read() != -1);
		
	if(_buffer[nRead] == '\n'&&_wpos) {
		memcpy(buf,_buffer,nRead);
		if (buf[nRead - 1] == '\r') {
			buf[nRead - 1] = 0;
			nRet = nRead - 2;
		}
		else {
			buf[nRead] = 0;
			nRet = nRead - 1;
		}
		nRead += 1;
		
		//LOGOUT("+++info+++ %d,%d\n",nRead,_wpos);
		_wpos -= nRead;
		copy_iner_buffer(_buffer,nRead,0,_wpos);
		//fix serial port read error block
		if(_wpos == 0)
			_buffer[0] = 0;

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
    vsprintf(buf,s, argptr);  
    va_end(argptr);  
	nWrite = strlen(buf);
	return write(buf,nWrite);
}

int Serial::println(const char* s,...) {
	char buf[256];
	int nWrite = -1;
	
	va_list argptr;  
    va_start(argptr, s);  
    vsprintf(buf,s, argptr);  
    va_end(argptr);  
	nWrite = strlen(buf);
	buf[nWrite]='\n';
	return write(buf,nWrite+1);
}

#if !defined(RESET_GPIO_PORT)
bool Serial::touchForCDCReset() {
	if(begin(_B115200) == false)
		return false;
	int status;
	
	LOGOUT("***INFO*** touchForCDCReset\n");
	ioctl(_serial_fd, TIOCMGET, &status);
	status &= ~TIOCM_DTR;
	ioctl(_serial_fd, TIOCMSET, &status);

	close();
	return true;
}
#else
bool Serial::touchForCDCReset() {
	int fd;
	int i=0;
	fd = open("/sys/class/gpio/export",O_WRONLY);
	if(fd == -1)
		return false;
	::write(fd,RESET_GPIO_PORT,2);
	::close(fd);
	
	fd = open(RESET_GPIO_DERICTION,O_WRONLY);
	if(fd == -1)
		return false;
	::write(fd,"out",3);
	::close(fd);
	
	fd = open(RESET_GPIO_VALUE,O_WRONLY);
	if(fd == -1)
		return false;
	::write(fd,"0",1);
	usleep(10);
	::write(fd,"1",1);
	::close(fd);
	
	fd = open("/sys/class/gpio/unexport",O_WRONLY);
	if(fd == -1)
		return false;
	::write(fd,RESET_GPIO_PORT,2);
	::close(fd);
	return true;
}	
#endif

#endif
