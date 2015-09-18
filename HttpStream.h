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

	int read(u8* buf,int len);
	
	int readline(u8* buf,int len);
	
	int skip(int len);
	
	int write(const u8* wbuf,int len,bool blocked = false);
	
	int left();

	void alloc_new_page(int cnt);
	
	void setDown(bool v);
	
	bool getDown();
	
	int size();
	
	void clear();
private:
	int _page_size;
	list<u8*>* _free_page;
	list<u8*>* _p_arr;
	int _read_pos;
	int _write_pos;
	u8* _readpage_addr;
	u8* _writepage_addr;
	pthread_mutex_t _write_mutex;
	bool _isDown;
};

#endif