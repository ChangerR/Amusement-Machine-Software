#include "HttpUrlConnection.h"
#include "curl/curl.h"
#include "HttpStream.h"
#include <stdio.h>
#include <string.h>

#ifdef SLSERVER_WIN32
#pragma comment(lib,"libcurl.lib")
#endif

long HttpUrlConnection::data_writer_sync(void *data, int size, int nmemb, HttpStream* stream) { 
	return stream->write((u8*)data,size*nmemb,false);
}

long HttpUrlConnection::response_writer(void *data, int size, int nmemb, HttpUrlConnection* _urlconn) {
	long sizes = size * nmemb;
	if(_urlconn->_wpos + sizes >= 4096)
		return sizes;
	memcpy(_urlconn->_response + _urlconn->_wpos, (char*)data, sizes);
	_urlconn->_wpos += sizes;
	return sizes;
}

HttpUrlConnection::HttpUrlConnection(u32 page_size,u32 page_count){	
	outs = NULL;
	_wpos = 0;
	_page_size = page_size;
	_page_count = page_count;
	handler = curl_easy_init();
}
HttpUrlConnection::~HttpUrlConnection() {
	curl_easy_cleanup(handler);
	if (outs)
		delete outs;
}

bool HttpUrlConnection::open(const char* url) {
	
	_wpos = 0;
	memset(_response,0,4096);
	
	//outs->clear();
	if (outs)
		delete outs;

	outs = new HttpStream(_page_size, _page_count);

	curl_easy_setopt(handler, CURLOPT_URL, url);
	curl_easy_setopt(handler, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(handler,CURLOPT_FOLLOWLOCATION,1);
	/// 保存body文件
	curl_easy_setopt(handler, CURLOPT_WRITEFUNCTION, HttpUrlConnection::data_writer_sync);
	curl_easy_setopt(handler, CURLOPT_WRITEDATA, outs);
	/// 保存服务器返回的响应消息
	curl_easy_setopt(handler, CURLOPT_HEADERFUNCTION, HttpUrlConnection::response_writer);
	curl_easy_setopt(handler, CURLOPT_WRITEHEADER, this);
	CURLcode res = CURLE_OK;
	int retcode = 0;
	res = curl_easy_perform(handler);
	res = curl_easy_getinfo(handler, CURLINFO_RESPONSE_CODE , &retcode);
	
	outs->setDown(true);
	
	if ( (res == CURLE_OK) && retcode == 200 ) {		
		return true;
	}
	else
		return false;
}

HttpStream* HttpUrlConnection::getHttpOutputStream(){
	return outs;
}

const char* HttpUrlConnection::getResponse() {
	return _response;
}
