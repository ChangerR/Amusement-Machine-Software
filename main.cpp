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
#include <signal.h>

int getopt(int argc, char * const argv[], const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

#endif

SlGlobal slglobal;

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

#ifdef SLSERVER_LINUX
void signal_handler(int sig) {
	printf("now we get signal Ctrl-C to stop\n");
	slglobal.global_running = false;
}
#endif

int main(int args,char** argv) {
	int ch = 0;
	int port = 0;
	char serial_port[32] = { 0 };
	char filename[256] = {0};
	ServerConfig* pConfig = NULL;
	SlServer* pserver = NULL;
	
#ifdef SLSERVER_LINUX
	if(signal(SIGINT, signal_handler) == SIG_ERR) {
        printf("could not register signal handler\n");
        exit(1);
    }
	pthread_mutex_init(&slglobal.frame_lock,NULL);
#else
	INIT_GLOBAL_FRAME_LOCK;
#endif

	slglobal.is_stream_running = false;
	slglobal.frame = NULL;
	slglobal.frame_size = 0;
	slglobal.frame_alloc_size = 0;
	slglobal.frame_count = 1;
	
	
	
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
	
	LOGOUT("***INFO*** now we start server\n");
	
	pserver = new SlServer(port,pConfig);
	slglobal.pConfig = pConfig;
	
	if (pserver->init(serial_port) == false) {
		goto end;
	}

	if (pserver->start() == false) {
		goto end;
	}

	LOGOUT("***INFO*** server start ok\n");
	slglobal.server = pserver;
	
#ifdef SLSERVER_WIN32
	LOGOUT("***INFO*** Please Input A Char To Stop This Server\n");
	getc(stdin);
#else
	slglobal.global_running = true;

	while(slglobal.global_running) {
		usleep(10);
	}
#endif

	pserver->stop();
	if (pserver)
		delete pserver;
end:
	if (pConfig){
		pConfig->sync();
		delete pConfig;
	}
#ifdef SLSERVER_LINUX
	pthread_mutex_destroy(&slglobal.frame_lock);
#else
	DESTROY_GLOBAL_FRAME_LOCK;
#endif
	if(slglobal.frame)
		 free(slglobal.frame);
	
	return 0;
}

