#ifndef __SLSERVER_HTTP_CONNECTION_
#define __SLSERVER_HTTP_CONNECTION_
#include "slconfig.h"
#include "curl/curl.h"
#include "HttpStream.h"
class HttpUrlConnection {
public:
	HttpUrlConnection(u32,u32);
	virtual ~HttpUrlConnection();
	
	bool open(const char*);
	const char* getResponse();
	HttpStream* getHttpOutputStream();
	
	static long data_writer_sync(void *data, int size, int nmemb, HttpStream* stream);
	static long response_writer(void *data, int size, int nmemb, HttpUrlConnection* _urlconn);
private:
	HttpStream* outs;
	char _response[4096];
	int _wpos;
	CURL* handler;
	u32 _page_size, _page_count;
};
#endif