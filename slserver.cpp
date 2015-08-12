#include "slserver.h"
#include <time.h>
#include "slconfig.h"
#include "gopro_plan_queue.h"

#ifdef SLSERVER_WIN32 
#include <iphlpapi.h>
#include <windows.h>
#else
#include <sys/time.h>
#endif
#ifdef __linux__
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h> 
#include <netinet/if_ether.h> 
#endif

#ifdef SLSERVER_WIN32
#pragma comment(lib,"pthreadVC2.lib")
#pragma comment(lib,"Iphlpapi")
#endif
#ifdef SLSERVER_WIN32
#pragma comment(lib,"wsock32.lib")
int gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);
    tm.tm_year     = wtm.wYear - 1900;
    tm.tm_mon     = wtm.wMonth - 1;
    tm.tm_mday     = wtm.wDay;
    tm.tm_hour     = wtm.wHour;
    tm.tm_min     = wtm.wMinute;
    tm.tm_sec     = wtm.wSecond;
    tm.tm_isdst    = -1;
    clock = mktime(&tm);
    tp->tv_sec = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;

    return (0);
}
#endif


SlServer::SlServer(int p,ServerConfig* pconfig) {
	poilt = NULL;
	port = p;
	server = INVALID_SOCKET;
	_config = pconfig;
	_planqueue = new GoproPlanQueue;
#ifdef SLSERVER_WIN32
	if(socket_init <= 0) {
		socket_init = 0;
		winsock_init();
	}
	socket_init += 1;
#endif
}

SlServer::~SlServer() {
#ifdef SLSERVER_WIN32
	socket_init -= 1;
	if(socket_init <= 0) {
		winsock_close();
	}
#endif
	if (_planqueue)
		delete _planqueue;
	if(poilt) {
		poilt->stop();
		delete poilt;
	}

}


bool SlServer::init(const char* p) {
	struct sockaddr_in sockaddr;
	
	poilt = new slrov(this);

	if(!poilt->start(p)) {
		return false;
	}
	
	server = socket(AF_INET,SOCK_STREAM,0/*IPPROTO_TCP*/);
	
	if(server == INVALID_SOCKET) {
		LOGOUT("can not open socket\n");				
		return false;	
	}
	
	sockaddr.sin_family = AF_INET;
#ifdef SLSERVER_WIN32
	sockaddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#else
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
	sockaddr.sin_port = htons(port);
	
	if(SOCKET_ERROR == bind(server,(SOCKADDR*)&sockaddr,sizeof(SOCKADDR_IN))) {
		closesocket(server);
		LOGOUT("bind address failed,please check\n");
		return false;
	}
	
	if(SOCKET_ERROR == listen(server,SOMAXCONN)) {
		closesocket(server);
		LOGOUT("socket listen failed,please check\n");
		return false;
	}

	LOGOUT("***INFO*** Now We Listen at port %d\n", port);

	gopro4::_init_mac = _config->getMacAddress("GOPRO_MAC", gopro4::_smac);

	return true;
}

bool SlServer::start() {
	running = true;
	watch_running = true;
	
	if (!_planqueue->start())
		return false;

	if (0 != pthread_create(&_thread, NULL, SlServer::recv_data, (void*)this)) {
		printf("error when create pthread,%d\n", errno);
		return false;
	}
	
	pthread_mutex_init(&_timer_int_mutex, NULL);
	pthread_cond_init(&_timer_int_cond, NULL);
	pthread_mutex_init(&_clients_mutex,NULL);
	pthread_mutex_init(&_clients_write_mutex,NULL);
	
	if (0 != pthread_create(&_timer_thread, NULL, SlServer::watcher, (void*)this)) {
		printf("error when create pthread watcher thread,%d\n", errno);
		return false;
	}
	
	return true;
}

void SlServer::stop() {
	running = false;
	watch_running = false;
	
	pthread_mutex_lock(&_timer_int_mutex);
	pthread_cond_signal(&_timer_int_cond);
	pthread_mutex_unlock(&_timer_int_mutex);
	
	pthread_join(_thread, NULL);
	pthread_join(_timer_thread,NULL);
	
	pthread_mutex_destroy(&_timer_int_mutex);
	pthread_mutex_destroy(&_clients_mutex);
	pthread_mutex_destroy(&_clients_write_mutex);
	
}

void* SlServer::watcher(void* data) {
	SlServer* pointer = (SlServer*)data;
	struct timeval now;
	struct timespec outtime;
	
	pthread_mutex_lock(&pointer->_timer_int_mutex);
	
	while(pointer->watch_running) {
		gettimeofday(&now, NULL);
		outtime.tv_sec = now.tv_sec + 10;
		outtime.tv_nsec = now.tv_usec * 1000;
		pthread_cond_timedwait(&pointer->_timer_int_cond, &pointer->_timer_int_mutex, &outtime);
		
		pthread_mutex_lock(&pointer->_clients_mutex);
		for(list<SlClient*>::node * p = pointer->clients.begin(); p != pointer->clients.end();p = p->next) {
			p->element->time += 10;
			//LOGOUT("***INFO*** UID:%d client add time %d\n", p->element->uid, p->element->time);
			if(p->element->time >= 60) {
				p->element->client_state = CLIENT_DEAD;
				LOGOUT("***INFO*** UID:%d client deaded\n", p->element->uid);
			}
		}
		pthread_mutex_unlock(&pointer->_clients_mutex);
	}
	
	pthread_mutex_unlock(&pointer->_timer_int_mutex);
	LOGOUT("***INFO*** CLIENT TIMER END\n");
	return NULL;
}

void* SlServer::recv_data(void* data) {
	SlServer* pointer = (SlServer*)data;
	char msg[1024] = {0};
	struct timeval timeout;
	int sel_ret;

	int maxsock = pointer->server;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	while(pointer->running) {
		
		FD_ZERO(&pointer->receiveQueue);
		FD_SET(pointer->server,&pointer->receiveQueue);
		maxsock = pointer->server;	
		pthread_mutex_lock(&pointer->_clients_mutex);
		for(list<SlClient*>::node * p = pointer->clients.begin(); p != pointer->clients.end();p = p->next) {
			
			if(p->element->client_state == CLIENT_DEAD) {
				pointer->_planqueue->_gopro4->removeClient(p->element->uid);
				list<SlClient*>::node* temp = p->prev;
				p->element->close();
				delete p->element;
				pointer->clients.erase(p);
				p = temp;
			}else {
				FD_SET(p->element->sock, &pointer->receiveQueue);
				if(p->element->sock > maxsock)
					maxsock = p->element->sock;
			}
		}
		pthread_mutex_unlock(&pointer->_clients_mutex);
		
		if (SOCKET_ERROR == (sel_ret = select(maxsock + 1,&pointer->receiveQueue, NULL, NULL,&timeout))) {
			LOGOUT("**ERROR*** slserver select function error\n");
			return NULL;
		}
		
		if(sel_ret && FD_ISSET(pointer->server,&pointer->receiveQueue)&&pointer->clients.getSize() < 64) {
			SOCKET accept_sock = INVALID_SOCKET;
			
			if(INVALID_SOCKET != (accept_sock = accept(pointer->server,NULL,NULL))) {
				SlClient* client = new SlClient(accept_sock);
				pthread_mutex_lock(&pointer->_clients_mutex);
				pointer->clients.push_back(client);
				pthread_mutex_unlock(&pointer->_clients_mutex);

				LOGOUT("***INFO*** UID:%d Client IP:%s Have Connected\n",client->uid,client->ip);
			} else {
				LOGOUT("**ERROR*** server accept client error,plese check\n");
			}		
		}
		
		for(list<SlClient*>::node * p = pointer->clients.begin(); p != pointer->clients.end();p = p->next) {
			
			if(FD_ISSET(p->element->sock,&pointer->receiveQueue) ) {
				if(p->element->client_state == CLIENT_WORKING) {
					int msg_len = 0;
					if (p->element->read() == -1) {
						LOGOUT("**ERROR*** Client Read Error\n");
						p->element->client_state = CLIENT_DEAD;
						continue;
					}
					while( -1 != (msg_len = p->element->getline(msg,1024))) {
						char* cp = msg;
						if(msg_len < 4|| cp[1] != ':' || cp[2] != ':' || cp[3] != ':') {
							LOGOUT("**ERROR*** Cmd:%s\n",msg);
							continue;
						}
						switch(*cp) {
							case '0':
								p->element->client_state = CLIENT_DEAD;
								pthread_mutex_lock(&pointer->_clients_write_mutex);
								p->element->write("0:::\r\n",6);
								pthread_mutex_unlock(&pointer->_clients_write_mutex);
								break;
							case '1':
								pthread_mutex_lock(&pointer->_clients_mutex);
								p->element->time = 0;
								pthread_mutex_unlock(&pointer->_clients_mutex);
								pthread_mutex_lock(&pointer->_clients_write_mutex);
								p->element->write("1:::\r\n",6);
								pthread_mutex_unlock(&pointer->_clients_write_mutex);
								//LOGOUT("***INFO*** UID:%d Client timer clear\n", p->element->uid);
								break;
							case '2':
								LOGOUT("***INFO*** recv msg %s\n", msg);
								pointer->poilt->runcommand(p->element->uid, cp + 4);
								break;
							case '3':
								if (!strncmp(cp + 4,"ping:",5))
								{
									char _tmps[32];
									sprintf_s(_tmps,"pong:%s",cp + 9);
									pointer->send(p->element->uid, 3, _tmps);
								}
								break;
							case '9':
								if (strcmp(cp + 4, "listen") == 0) {
									LOGOUT("***INFO*** gopro listen:%s\n", p->element->ip);
									pointer->_planqueue->_gopro4->addClient(p->element->uid, p->element->ip);
									pointer->send(p->element->uid, 2, "stream_on");
								}
								else
									pointer->_planqueue->addPlan(cp + 4);
								break;
							default:
								break;
						}
					}
				}else{
					if(p->element->read() == -1) {
						LOGOUT("when handsank socket read error\n");
						p->element->client_state = CLIENT_DEAD;
					}else if(p->element->handsank()) {	
						pthread_mutex_lock(&pointer->_clients_write_mutex);
						p->element->write("SLOK:::\r\n",9);
						pthread_mutex_unlock(&pointer->_clients_write_mutex);
					}else if(p->element->http_connect()) {
						
					}
				}
			}
		}
#ifdef SLSERVER_WIN32
		Sleep(1);
#else
		usleep(1);
#endif
	}

	LOGOUT("***INFO*** DATA RECEIVER END\n");
	return NULL;
}

void SlServer::broadcast(int channel,const char* p) {
	static char b_buf[1024];
	int len;
	
	sprintf_s(b_buf, "%d:::%s\r\n", channel, p);
	len = strlen(b_buf);
	
	pthread_mutex_lock(&_clients_write_mutex);
//	LOGOUT("***DEBBUG*** BROADCAST\n");
	for(list<SlClient*>::node * pn = clients.begin(); pn != clients.end();pn = pn->next) {
		pn->element->write(b_buf,len);
		//LOGOUT("***INFO*** write:%d cmd:%s", pn->element->uid, b_buf);
	}

	pthread_mutex_unlock(&_clients_write_mutex);
}

void SlServer::send(int id,int channel,const char* ps) {
	static char b_buf[1024];
	int len;
	
	sprintf_s(b_buf, "%d:::%s\r\n", channel, ps);
	len = strlen(b_buf);
	
	for(list<SlClient*>::node * p = clients.begin(); p != clients.end();p = p->next) {
		if(p->element->uid == id) {
			pthread_mutex_lock(&_clients_write_mutex);
			if (p->element->write(b_buf, len) == -1) {
				LOGOUT("***ERR***: Send UID:%d error", id);
			}			
			pthread_mutex_unlock(&_clients_write_mutex);
			//LOGOUT("***INFO***: Send UID:%d %s", id, b_buf);
			break;
		}
	}
}

#ifdef SLSERVER_WIN32

bool SlServer::getMacAddr(const char* ip, unsigned char* mac) {
	unsigned long targetIp = inet_addr(ip);
	unsigned long macBuf[2];
	unsigned long macLen = 6;
	unsigned long localip = inet_addr("0.0.0.0");

	unsigned long retValue = SendARP(targetIp, localip, macBuf, &macLen);

	if (retValue != NO_ERROR){
		fprintf(stderr, "SendARP error/n");
		return false;
	}

	if (macLen == 0) {
		printf("***ERROR*** get mac error\n");
		return false;
	}
	memcpy(mac, macBuf, macLen);
	return true;
}
#endif

#ifdef SLSERVER_LINUX
bool SlServer::getMacAddr(const char* ip, unsigned char* mac) {
	int sd;
	struct arpreq arpreq;
	struct sockaddr_in *sin;
	struct in_addr ina;
	unsigned char *hw_addr;

	int rc;

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		printf("socket() error\n");
		return false;
	}

	/* Try to find an entry in arp cache for the ip address specified */


	printf("Find arp entry for IP : %s\n", ip);

	/*you must add this becasue some system will return "Invlid argument"
	because some argument isn't zero */

	memset(&arpreq, 0, sizeof(struct arpreq));

	sin = (struct sockaddr_in *) &arpreq.arp_pa;
	memset(sin, 0, sizeof(struct sockaddr_in));
	sin->sin_family = AF_INET;
	ina.s_addr = inet_addr(ip);
	memcpy(&sin->sin_addr, (char *)&ina, sizeof(struct in_addr));

	strcpy(arpreq.arp_dev, "wlan0");

	rc = ioctl(sd, SIOCGARP, &arpreq);

	if (rc < 0) {
		printf("Entry not available in cache...\n");
		return false;
	} else {
		printf("\nentry has been successfully retreived\n");
		hw_addr = (unsigned char *) arpreq.arp_ha.sa_data;

		printf("HWAddr found : %02x:%02x:%02x:%02x:%02x:%02x\n",
		       hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3], hw_addr[4], hw_addr[5]);
		
		memcpy(mac,hw_addr,6);
	}
	return true;
}
#endif

#ifdef SLSERVER_WIN32
int SlServer::socket_init = 0;

bool SlServer::winsock_init() {
	WSADATA wsadata;
	if( WSAStartup( MAKEWORD( 2, 2 ), &wsadata ) )
	{
		LOGOUT( "initializationing error!\n" );
		WSACleanup( );
		return false;
	}
	
	return true;
}

void SlServer::winsock_close() {
	WSACleanup( );
}
#endif
