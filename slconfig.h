#ifndef __SLCONFIG__H
#define __SLCONFIG__H

typedef unsigned char byte;
typedef unsigned int  u32;
typedef int s32;
typedef unsigned char u8;
typedef char s8;

#ifdef _WIN32
#define SLSERVER_WIN32
#elif defined(__linux__)
#define SLSERVER_LINUX
#else
#error "error platform"
#endif

#include <stdio.h>
#define LOGOUT printf

#define HANDASNK_MAGIC 0x41564F52

#ifdef SLSERVER_LINUX
#define strcpy_s strcpy
#define sprintf_s sprintf
#define __cdecl
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKADDR_IN struct sockaddr_in
#define SOCKADDR struct sockaddr
#define SOCKET_ERROR -1
#define closesocket ::close
#endif
#define BBBLACK_GPIO30_RESET
#endif
