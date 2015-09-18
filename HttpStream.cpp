#include "HttpStream.h"
#include <stdlib.h>
#include <string.h>
#ifdef SLSERVER_WIN32
#include <windows.h>
#endif
#ifdef SLSERVER_LINUX
#include <unistd.h>
#endif

HttpStream::HttpStream(int size_count,int cnt) :_page_size(size_count),_isDown(false){

	pthread_mutex_init(&_write_mutex, NULL);
	_free_page = new list<u8*>();
	_p_arr = new list<u8*>();
	u8* temp = NULL;
	//_p_arr->push_back(new u8[size_count]);
	for(int i = 0;i < cnt - 1;i ++) {
		_free_page->push_back(temp);
	}

	//_read_pos = 0;
	//_write_pos = 0;
	//_readpage_addr = _p_arr->begin()->element;
	//_writepage_addr = _readpage_addr;
	
	clear();
}

HttpStream::~HttpStream() {
	lock();
	for(list<u8*>::node* p = _free_page->begin();p != _free_page->end();p = p->next) {
		if (p->element)
			delete [] p->element;
	}
	for(list<u8*>::node* p = _p_arr->begin();p != _p_arr->end();p = p->next) {
		delete [] p->element;
	}
	delete _free_page;
	delete _p_arr;
	unlock();
	
	pthread_mutex_destroy(&_write_mutex);
}


void HttpStream::lock() {
	pthread_mutex_lock(&_write_mutex);
}

void HttpStream::unlock() {
	pthread_mutex_unlock(&_write_mutex);
}
	
int HttpStream::read(u8* buf,int len) {
	int read_len = 0;
	int span = 0;
	lock();
	while(len) {
		span = (_readpage_addr == _writepage_addr ? _write_pos : _page_size) - _read_pos;
		span = span > len ? len:span;
		if (span <= 0)
		{
			if(_readpage_addr == _writepage_addr)
				break;
			_read_pos = 0;
			memset(_readpage_addr,0,_page_size);
			_free_page->push_back(_readpage_addr);
			_p_arr->erase(0);
			_readpage_addr = _p_arr->begin()->element;
			continue;
		}
		memcpy(buf,_readpage_addr + _read_pos,span);
		_read_pos += span;
		read_len += span;
		buf += span;
		len -= span;
	}
	unlock();
	return read_len;
}

int HttpStream::readline(u8* buf,int len) {
	char* p = NULL;
	char* end ;
	int read_size = 1;	
	lock();
	for(list<u8*>::node* p_node = _p_arr->begin();p_node != _p_arr->end();p_node = p_node->next) {
		p = (char*)p_node->element + (p_node->element == _readpage_addr ?_read_pos:0);
		end = (char*)p_node->element + (p_node->element == _writepage_addr ?_write_pos:_page_size);
		while(p < end&&*p != '\n')p++;
		read_size += p - ((char*)p_node->element + (p_node->element == _readpage_addr ?_read_pos:0));
		if(read_size > len) {
			unlock();
			return -2;
		}
		if(p < end)
			break;
	}
	unlock();
	if (p&&*p != '\n') {
		return -1;
	}
	return read(buf,read_size);
}
	
int HttpStream::write(const u8* wbuf,int len,bool blocked) {
	int write_len = 0;
	int span;
	lock();
	while (len > 0)
	{
		span = _page_size - _write_pos;
		if (span <= 0)
		{	
			if(blocked) {
				while (!_free_page->getSize()) {
					unlock();
#ifdef SLSERVER_WIN32
					Sleep(50);
#elif defined(SLSERVER_LINUX)
					usleep(50);
#endif
					lock();
				}
			}else if(!_free_page->getSize())
				break;
			
			_writepage_addr = _free_page->begin()->element;
			if (_writepage_addr == NULL)
				_writepage_addr = new u8[_page_size];
			_free_page->erase(0);
			_p_arr->push_back(_writepage_addr);
			_write_pos = 0;
		}
		span = span > len ? len:span;
		memcpy(_writepage_addr+_write_pos,wbuf,span);
		_write_pos += span;
		write_len += span;
		wbuf += span;
		len -= span;
	}
	unlock();
	return write_len;
}

int HttpStream::left() {
	lock();
	int ret = _free_page->getSize()*_page_size + _page_size - _write_pos;
	unlock();
	return ret;
}

void HttpStream::setDown(bool v)
{
	lock();
	_isDown = v;
	unlock();
}

bool HttpStream::getDown()
{
	return _isDown;
}


void HttpStream::clear() {
	
	lock();
	
	for(list<u8*>::node* p = _p_arr->begin();p != _p_arr->end();p = p->next) {
		_free_page->push_front(p->element);
	}

	_writepage_addr = _free_page->begin()->element;

	if (_writepage_addr == NULL)
		_writepage_addr = new u8[_page_size];

	_free_page->erase(0);
	_p_arr->push_back(_writepage_addr);
	_readpage_addr = _writepage_addr;

	_read_pos = _write_pos = 0;
	
	unlock();
	
}

void HttpStream::alloc_new_page( int cnt )
{
	u8* temp = NULL;
	lock();
	for(int i = 0;i < cnt;i ++) {
		_free_page->push_back(temp);
	}
	unlock();
}

int HttpStream::skip( int len )
{
	int read_len = 0;
	int span = 0;
	lock();
	while(len) {
		span = (_readpage_addr == _writepage_addr ? _write_pos : _page_size) - _read_pos;
		span = span > len ? len:span;
		if (span <= 0)
		{
			if(_readpage_addr == _writepage_addr)
				break;
			_read_pos = 0;
			memset(_readpage_addr,0,_page_size);
			_free_page->push_back(_readpage_addr);
			_p_arr->erase(0);
			_readpage_addr = _p_arr->begin()->element;
			continue;
		}
		_read_pos += span;
		read_len += span;
		len -= span;
	}
	unlock();
	return read_len;
}

int HttpStream::size() {
	lock();
	int ret = _p_arr->getSize()*_page_size + _write_pos;
	unlock();
	return ret;
}
