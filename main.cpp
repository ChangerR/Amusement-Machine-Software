#include "serial.h"
#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "slserver.h"
#include "hardware.h"
#include "getopt.h"
#include "ServerConfig.h"
#include "HttpStream.h"
#include "HttpUrlConnection.h"
#include "gopro4.h"
#ifdef SLSERVER_LINUX
#include <unistd.h>
int getopt(int argc, char * const argv[], const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

#endif

Global global;

/*
int main(int args,char** argv) {

	gopro4 _gopro;
	
	if (_gopro.init() == false)
	{
		return -1;
	}

	if (_gopro.start() == false)
	{
		return -1;
	}

	LOGOUT("***INFO*** Please Input A Char To Stop This Server\n");
	getc(stdin);

	_gopro.stop();

	return 1;
}
*/


void Usage() {
	LOGOUT("Usage: slserver -p [port] -s [com] -f [config file]\n");
}

int main(int args,char** argv) {
	int ch = 0;
	int port = 0;
	char serial_port[32] = { 0 };
	char filename[256] = {0};
	ServerConfig* pConfig = NULL;
	SlServer* pserver = NULL;
	
	
	pthread_mutex_init(&global.frame_lock,NULL);
	global.is_stream_running = false;
	global.frame = NULL;
	global.frame_size = 0;
	global.frame_alloc_size = 0;
	
	while((ch = getopt(args,argv,"p:s:f:")) != -1) {
		
		switch(ch) {
			case 'p':
				port = parse_int_dec(optarg);
				break;
			case 's':
				strcpy_s(serial_port,optarg);
				break;
			case 'f':
				strcpy_s(filename,optarg);
				break;
			default:
				LOGOUT("%s:Unknow Option -%c\n",argv[0],ch);
				goto end;
		}
	}
	
	if(*filename == 0) {
		Usage();
		LOGOUT("\t you must input config file\n");
		goto end;
	}
	
	pConfig = new ServerConfig();
	pConfig->init(filename);
	
	if(pConfig->parseData() == false) {
		LOGOUT("***ERROR*** error config file\n");
		goto end;
	}
	pConfig->debug();
	
	if(port == 0&&!pConfig->getInt("port",&port)) {
		LOGOUT("***ERROR*** Cannot find port\n");
		goto end;
	}
	
	if(*serial_port == 0&&!pConfig->getString("serial",serial_port)) {
		LOGOUT("***ERROR*** Cannot define serial port\n");
		goto end;
	}

	
	pserver = new SlServer(port,pConfig);
	global.pConfig = pConfig;
	
	if (pserver->init(serial_port) == false) {
		goto end;
	}

	if (pserver->start() == false) {
		goto end;
	}

	LOGOUT("***INFO*** Please Input A Char To Stop This Server\n");
	getc(stdin);

	pserver->stop();
	if (pserver)
		delete pserver;
end:
	if (pConfig){
		pConfig->sync();
		delete pConfig;
	}
	
	pthread_mutex_destroy(&global.frame_lock);
	if(global.frame)
		delete[] global.frame;
	
	return 0;
}

