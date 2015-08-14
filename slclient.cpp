#include "slclient.h"
#include "slconfig.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "global.h"

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

static const struct {
    const char *dot_extension;
    const char *mimetype;
} mimetypes[] = {
    { ".html", "text/html" },
    { ".htm",  "text/html" },
    { ".css",  "text/css" },
    { ".js",   "text/javascript" },
    { ".txt",  "text/plain" },
    { ".jpg",  "image/jpeg" },
    { ".jpeg", "image/jpeg" },
    { ".png",  "image/png"},
    { ".gif",  "image/gif" },
    { ".ico",  "image/x-icon" },
    { ".swf",  "application/x-shockwave-flash" },
    { ".cab",  "application/x-shockwave-flash" },
    { ".jar",  "application/java-archive" },
    { ".json", "application/json" },
	{ ".config", "text/plain" },
	{ ".ttf", "application/octet-stream" },
	{ ".svg", "image/svg+xml" },
	{ ".woff","application/x-font-woff"}
};

u32 SlClient::id_counter = 1000;

inline void copy_socket_buffer(char* buf,int from,int to,int len) {
	for(int i = 0; i < len;i++) {
		buf[to + i] = buf[from + i];
	}
}

int SlClient::getline_no_remove(char* buf,int max_len) {
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
			nRead -= 1;
		}
		else {
			buf[nRead] = 0;
		}
		return nRead;
	}
	
	return -1;
}

int SlClient::getline(char* buf,int max_len) {
	int nRead = 0,len;
	for(;nRead < _wpos;nRead++) {
		if(_buffer[nRead] == '\n')
			break;
	}
	
	if(_buffer[nRead] == '\n') {
		if(nRead > max_len) {
			LOGOUT("***ERROR*** string big than max len\n");
			return -1;
		}
		memcpy(buf,_buffer,nRead);
		if (buf[nRead - 1] == '\r') {
			buf[nRead - 1] = 0;
			len = nRead - 1;
		}
		else {
			buf[nRead] = 0;
			len = nRead;
		}
		nRead++;
		_wpos -= nRead;
		copy_socket_buffer(_buffer,nRead,0,_wpos);
		return len;
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
	
	if(-1 == getline_no_remove(hand_buf,256)) {
		LOGOUT("when handsank occur error format\n");
		client_state = CLIENT_DEAD;
		return false;
	}
	
	magic = *(u32*)((void*)hand_buf);
	
	if(magic != HANDASNK_MAGIC) {
//		LOGOUT("handsank error magic NUMBER:0x%X\n",magic);
		client_state = CLIENT_DEAD;
		return false;
	}
	
	if(p[4] != ':'||p[5] != ':'||p[6] != ':') {
//		LOGOUT("handsank error format");
		client_state = CLIENT_DEAD;
		return false;
	}
	getline(hand_buf,256);
	client_state = CLIENT_WORKING;
	return true;
}

bool SlClient::http_connect() {
	char hand_buf[256];
	char* p = hand_buf;
	int offset = 0;
	bool ret = false;
	if(-1 == getline_no_remove(hand_buf,256)) {
		LOGOUT("when handsank occur error format\n");
		client_state = CLIENT_DEAD;
		return false;
	}
	
	do {
		while(*p != ' '&&*p) {
			if(offset >= METHOD_NAME_LEN - 1) {
				LOGOUT("======>ERROR Http Method\n");
				break;
			}
			_method[offset++] = *p++;
		}
		_method[offset] = 0;
		offset = 0;
		
		if(*p != ' ')
			break;
		p++;

		while(*p != ' '&&*p) {
			if(offset >= REQUEST_PATH - 1) {
				LOGOUT("======>ERROR Http Path\n");
				break;
			}
			_path[offset++] = *p++;
		}
		_path[offset] = 0;
		offset = 0;
		
		if(*p != ' ')
			break;
		p++;

		while(*p) {
			if(offset >= REQUEST_MAGIC - 1) {
				LOGOUT("======>ERROR Http Version\n");
				break;
			}
			_magic[offset++] = *p++;
		}
		_magic[offset] = 0;
		
		if(!*p) {
			client_state = CLIENT_HTTP;
			ret = true;
		}
	}while(0);
	
	getline(hand_buf,256);
	return ret;
}

bool SlClient::do_http() {
	if (0 != pthread_create(&_thread, NULL, SlClient::http_client_thread, (void*)this)) {
		printf("=======>error when create http pthread,%d\n", errno);
		return false;
	}
	
	return true;
}

void* SlClient::http_client_thread(void* arg) {
	SlClient* pointer = (SlClient*)arg;
	char buf[512] = {0};
	int ret = 0;
	do {
		if(0 != strcmp("GET",pointer->_method))
			break;
		
		if(0 != strcmp("HTTP/1.1",pointer->_magic))
			break;
		
		while((ret = pointer->getline(buf,512)) > 0) {
			LOGOUT("====>%s\n",buf);
		}
		
		if(ret != 0) {
			LOGOUT("====>Read client error\n");
			break;
		}
		
		if(strcmp(pointer->_path,"/") == 0) {
			pointer->send_file("index.html");
		}else if(strcmp(pointer->_path,"/stream") == 0) {
			pointer->send_stream();
		}else {
			pointer->send_file(pointer->_path);
		}
		
	}while(0);
	
	pointer->client_state = CLIENT_DEAD;
	return NULL;
}
	
void SlClient::send_error(int which,const char* message) {
	char buffer[1024] = {0};

    if(which == 401) {
        sprintf_s(buffer, "HTTP/1.1 401 Unauthorized\r\n" \
                "Content-type: text/plain\r\n" \
                STD_HEADER \
                "WWW-Authenticate: Basic realm=\"SlServer\"\r\n" \
                "\r\n" \
                "401: Not Authenticated!\r\n" \
                "%s", message);
    } else if(which == 404) {
        sprintf_s(buffer, "HTTP/1.1 404 Not Found\r\n" \
                "Content-type: text/plain\r\n" \
                STD_HEADER \
                "\r\n" \
                "404: Not Found!\r\n" \
                "%s", message);
    } else if(which == 500) {
        sprintf_s(buffer, "HTTP/1.1 500 Internal Server Error\r\n" \
                "Content-type: text/plain\r\n" \
                STD_HEADER \
                "\r\n" \
                "500: Internal Server Error!\r\n" \
                "%s", message);
    } else if(which == 400) {
        sprintf_s(buffer, "HTTP/1.1 400 Bad Request\r\n" \
                "Content-type: text/plain\r\n" \
                STD_HEADER \
                "\r\n" \
                "400: Not Found!\r\n" \
                "%s", message);
    } else {
        sprintf_s(buffer, "HTTP/1.1 501 Not Implemented\r\n" \
                "Content-type: text/plain\r\n" \
                STD_HEADER \
                "\r\n" \
                "501: Not Implemented!\r\n" \
                "%s", message);
    }
	
	if(write(buffer, strlen(buffer)) < 0) {
        LOGOUT("***ERROR*** write failed, done anyway\n");
    }
}

void SlClient::send_file(const char* parameter) {
	char buffer[1024] = {0};
    const char *extension, *mimetype = NULL;
    int i;
	FILE* lfd;
	int file_legth = 0;
	
	if(www_folder[0] == 0) {
		send_error(501, "Not set www_folder");
		return;
	}
	
    /* in case no parameter was given */
    if(parameter == NULL || strlen(parameter) == 0)
        parameter = "index.html";

    /* find file-extension */
    const char * pch;
    pch = strchr(parameter, '.');
    int lastDot = 0;
    while(pch != NULL) {
        lastDot = pch - parameter;
        pch = strchr(pch + 1, '.');
    }

    if(lastDot == 0) {
        send_error(400, "No file extension found");
        return;
    } else {
        extension = parameter + lastDot;
        LOGOUT("%s EXTENSION: %s\n", parameter, extension);
    }

    /* determine mime-type */
    for(i = 0; i < LENGTH_OF(mimetypes); i++) {
        if(strcmp(mimetypes[i].dot_extension, extension) == 0) {
            mimetype = (char *)mimetypes[i].mimetype;
            break;
        }
    }

    /* in case of unknown mimetype or extension leave */
    if(mimetype == NULL) {
        send_error(404, "MIME-TYPE not known");
        return;
    }

    /* now filename, mimetype and extension are known */
    LOGOUT("trying to serve file \"%s\", extension: \"%s\" mime: \"%s\"\n", parameter, extension, mimetype);

    /* build the absolute path to the file */
    strncat(buffer, www_folder, sizeof(buffer) - 1);
    strncat(buffer, parameter, sizeof(buffer) - strlen(buffer) - 1);

    /* try to open that file */
    if((lfd = fopen(buffer, "rb")) == NULL) {
        LOGOUT("file %s not accessible\n", buffer);
        send_error(404, "File not accessible");
        return;
    }
    LOGOUT("opened file: %s\n", buffer);
	fseek(lfd, 0, SEEK_END);
	file_legth = ftell(lfd);
	fseek(lfd, 0, SEEK_SET);

    /* prepare HTTP header */
    sprintf(buffer, "HTTP/1.1 200 OK\r\n" \
            "Content-type: %s\r\n" \
			"Content-length: %d\r\n" \
            STD_HEADER \
            "\r\n", mimetype,file_legth);
    i = strlen(buffer);

    /* first transmit HTTP-header, afterwards transmit content of file */
	if (write(buffer, i) < 0) {
		fclose(lfd);
		return;
	}

	while (file_legth > 0)
	{
		i = file_legth > 1024 ? 1024 : file_legth;
		fread(buffer, i, 1, lfd);

		if (write(buffer, i) < 0) {
			fclose(lfd);
			return;
		}
		file_legth -= i;
	}
    
    /* close file, job done */
    fclose(lfd);
}

void SlClient::send_stream() {
	unsigned char *frame = NULL;
    int frame_size = 0, max_frame_size = 0;
    char buffer[1024] = {0};
	long frame_id = 0;
	LOGOUT("preparing header\n");
    sprintf(buffer, "HTTP/1.1 200 OK\r\n" \
            "Access-Control-Allow-Origin: *\r\n" \
            STD_HEADER \
            "Content-Type: multipart/x-mixed-replace;boundary=" BOUNDARY "\r\n" \
            "\r\n" \
            "--" BOUNDARY "\r\n");

    if(write(buffer, strlen(buffer)) < 0) {
        return;
    }
	
	while(slglobal.is_stream_running) {
		
		if(slglobal.frame_size <= 0 || slglobal.frame == NULL)
			break;
		
		if(frame_id == slglobal.frame_count) {
#ifdef SLSERVER_WIN32
			Sleep(1);
#else
			usleep(1);
#endif
			continue;
		}
#ifdef SLSERVER_LINUX
		pthread_mutex_lock(&slglobal.frame_lock);
#else
		LOCK_GLOBAL_FRAME_LOCK;
#endif
		if(max_frame_size < slglobal.frame_size) {
			
			if(frame)
				free(frame);
			
			frame = (unsigned char* )malloc(slglobal.frame_alloc_size);
			max_frame_size = slglobal.frame_alloc_size;
		}
		
		memcpy(frame,slglobal.frame,slglobal.frame_size);
		frame_size = slglobal.frame_size;
		frame_id = slglobal.frame_count;
#ifdef SLSERVER_LINUX
		pthread_mutex_unlock(&slglobal.frame_lock);
#else
		UNLOCK_GLOBAL_FRAME_LOCK;
#endif
		sprintf(buffer, "Content-Type: image/jpeg\r\n" \
                "Content-Length: %d\r\n" \
                "\r\n", frame_size);
		if(write(buffer, strlen(buffer)) < 0) break;
		
		if(write((char*)frame, frame_size) < 0) break;

        sprintf(buffer, "\r\n--" BOUNDARY "\r\n");
		
        if(write(buffer, strlen(buffer)) < 0) break;
	}
	
	if(frame)
		 free(frame);
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

char SlClient::www_folder[256];
