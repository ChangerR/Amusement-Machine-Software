#include "HttpStream.h"
#include <stdlib.h>
#include <string.h>
#ifdef SLSERVER_WIN32
#include <windows.h>
#endif
#ifdef SLSERVER_LINUX
#include <unistd.h>
#endif
HttpStream::HttpStream(int size_count,int cnt) :page_size(size_count),isDown(false){
	free_page = new list<u8*>();
	p_arr = new list<u8*>();
	u8* temp = NULL;
	p_arr->push_back(new u8[size_count]);
	for(int i = 0;i < cnt - 1;i ++) {
		free_page->push_back(temp);
	}
	read_pos = 0;
	write_pos = 0;
	readpage_addr = p_arr->begin()->element;
	writepage_addr = readpage_addr;
	
	pthread_mutex_init(&_write_mutex,NULL);
}

HttpStream::~HttpStream() {
	lock();
	for(list<u8*>::node* p = free_page->begin();p != free_page->end();p = p->next) {
		if (p->element)
			delete [] p->element;
	}
	for(list<u8*>::node* p = p_arr->begin();p != p_arr->end();p = p->next) {
		delete [] p->element;
	}
	delete free_page;
	delete p_arr;
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
		span = (readpage_addr == writepage_addr ? write_pos : page_size) - read_pos;
		span = span > len ? len:span;
		if (span <= 0)
		{
			if(readpage_addr == writepage_addr)
				break;
			read_pos = 0;
			memset(readpage_addr,0,page_size);
			free_page->push_back(readpage_addr);
			p_arr->erase(0);
			readpage_addr = p_arr->begin()->element;
			continue;
		}
		memcpy(buf,readpage_addr + read_pos,span);
		read_pos += span;
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
	for(list<u8*>::node* p_node = p_arr->begin();p_node != p_arr->end();p_node = p_node->next) {
		p = (char*)p_node->element + (p_node->element == readpage_addr ?read_pos:0);
		end = (char*)p_node->element + (p_node->element == writepage_addr ?write_pos:page_size);
		while(p < end&&*p != '\n')p++;
		read_size += p - ((char*)p_node->element + (p_node->element == readpage_addr ?read_pos:0));
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
		span = page_size - write_pos;
		if (span <= 0)
		{	
			if(blocked) {
				while (!free_page->getSize()) {
					unlock();
#ifdef SLSERVER_WIN32
					Sleep(50);
#elif defined(SLSERVER_LINUX)
					usleep(50);
#endif
					lock();
				}
			}else if(!free_page->getSize())
				break;
			
			writepage_addr = free_page->begin()->element;
			if (writepage_addr == NULL)
				writepage_addr = new u8[page_size];
			free_page->erase(0);
			p_arr->push_back(writepage_addr);
			write_pos = 0;
		}
		span = span > len ? len:span;
		memcpy(writepage_addr+write_pos,wbuf,span);
		write_pos += span;
		write_len += span;
		wbuf += span;
		len -= span;
	}
	unlock();
	return write_len;
}

int HttpStream::left() {
	lock();
	int ret = free_page->getSize()*page_size + page_size - write_pos;
	unlock();
	return ret;
}

void HttpStream::setDown(bool v)
{
	lock();
	isDown = v;
	unlock();
}

bool HttpStream::getDown()
{
	return isDown;
}

/*
void HttpStream::clear() {
	
	lock();
	
	for(list<u8*>::node* p = p_arr->begin();p != p_arr->end();p = p->next) {
		free_page->push_back(p->element);
	}
	writepage_addr = free_page->begin()->element;
	if (writepage_addr == NULL)
		writepage_addr = new u8[page_size];
	free_page->erase(0);
	p_arr->push_back(writepage_addr);
	readpage_addr = writepage_addr;
	read_pos = write_pos = 0;
	
	unlock();
	
}
*/
void HttpStream::alloc_new_page( int cnt )
{
	u8* temp = NULL;
	lock();
	for(int i = 0;i < cnt;i ++) {
		free_page->push_back(temp);
	}
	unlock();
}

int HttpStream::skip( int len )
{
	int read_len = 0;
	int span = 0;
	lock();
	while(len) {
		span = (readpage_addr == writepage_addr ? write_pos : page_size) - read_pos;
		span = span > len ? len:span;
		if (span <= 0)
		{
			if(readpage_addr == writepage_addr)
				break;
			read_pos = 0;
			memset(readpage_addr,0,page_size);
			free_page->push_back(readpage_addr);
			p_arr->erase(0);
			readpage_addr = p_arr->begin()->element;
			continue;
		}
		read_pos += span;
		read_len += span;
		len -= span;
	}
	unlock();
	return read_len;
}
