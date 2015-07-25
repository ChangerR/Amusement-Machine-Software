#include "slclient.h"
#include "slconfig.h"
#include <string.h>
#ifdef SLSERVER_WIN32
#include <winsock2.h>
#endif
#ifdef __linux__
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#endif
u32 SlClient::id_counter = 1000;
inline void copy_socket_buffer(char* buf,int from,int to,int len) {
	for(int i = 0; i < len;i++) {
		buf[to + i] = buf[from + i];
	}
}

int SlClient::getline(char* buf,int max_len) {
	int nRead = 0;
	for(;nRead < _wpos;nRead++) {
		if(_buffer[nRead] == '\n')
			break;
	}
	
	if(_buffer[nRead] == '\n') {
		if(nRead > max_len)
			return -1;
		memcpy(buf,_buffer,nRead);
		if (buf[nRead - 1] == '\r') {
			buf[nRead - 1] = 0;
		}
		else {
			buf[nRead] = 0;
		}
		nRead++;
		_wpos -= nRead;
		copy_socket_buffer(_buffer,nRead,0,_wpos);
		return nRead - 1;
	}
	
	return -1;
}

int SlClient::write(const char* buf,int len) {
	if(SOCKET_ERROR == send(sock,buf,len,0)) 
		return -1;
	return len;
}

void SlClient::close() {
	closesocket(sock);
	LOGOUT("***INFO*** UID:%d client end\n", uid);
}

bool SlClient::handsank() {
	char hand_buf[256];
	char* p = hand_buf;
	u32 magic = 0;
	
	if(read() == -1) {
		LOGOUT("when handsank socket read error\n");
		client_state = CLIENT_DEAD;
		return false;
	}
	
	if(-1 == getline(hand_buf,256)) {
		LOGOUT("when handsank occur error format\n");
		client_state = CLIENT_DEAD;
		return false;
	}
	
	magic = *(u32*)((void*)hand_buf);
	
	if(magic != HANDASNK_MAGIC) {
		LOGOUT("handsank error magic NUMBER:0x%X\n",magic);
		client_state = CLIENT_DEAD;
		return false;
	}
	
	if(p[4] != ':'||p[5] != ':'||p[6] != ':') {
		LOGOUT("handsank error format");
		client_state = CLIENT_DEAD;
		return false;
	}

	client_state = CLIENT_WORKING;
	return true;
}

int SlClient::read() {
	int nRead = 0;
	
	if((nRead = recv(sock,_buffer + _wpos,CLIENT_BUFFER_LEN - _wpos,0)) == SOCKET_ERROR)
		return -1;
	_wpos += nRead;
	
	return _wpos;
}

SlClient::SlClient(SOCKET s)
{
	sock = s;
	time = 0;
	client_state = CLIENT_CREATING;
	_wpos = 0;
	uid = id_counter++;

	SOCKADDR_IN addr_conn;
#ifdef SLSERVER_WIN32
	int nSize = sizeof(addr_conn);
#else 
	socklen_t nSize = sizeof(addr_conn);
#endif	
	memset((void *)&addr_conn, 0, sizeof(addr_conn));

	if (sock != INVALID_SOCKET)
	{
		getpeername(sock, (SOCKADDR *)&addr_conn, &nSize);
		strcpy_s(ip, inet_ntoa(addr_conn.sin_addr));

	}else
		strcpy_s(ip, "0.0.0.0");
}
