#ifndef __SLSERVER_HTTPSTREAM_
#define __SLSERVER_HTTPSTREAM_
#include "slconfig.h"
#include "list.h"
#include <pthread.h>
class HttpStream {
public:
	HttpStream(int size_count,int cnt = 16);
	virtual ~HttpStream();
public:
	void lock();
	void unlock();
public:	
	int read(u8* buf,int len);
	int readline(u8* buf,int len);
	int skip(int len);
	int write(const u8* wbuf,int len,bool blocked = false);
	int left();

	void alloc_new_page(int cnt);
	void setDown(bool v);
	bool getDown();
	//void clear();
private:
	int page_size;
	list<u8*>* free_page;
	list<u8*>* p_arr;
	int read_pos;
	int write_pos;
	u8* readpage_addr;
	u8* writepage_addr;
	pthread_mutex_t _write_mutex;
	bool isDown;
};
#endif