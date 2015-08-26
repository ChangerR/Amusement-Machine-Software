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
#define strncpy_s strncpy
#define strcat_s strcat
#define __cdecl
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKADDR_IN struct sockaddr_in
#define SOCKADDR struct sockaddr
#define SOCKET_ERROR -1
#define closesocket ::close
#endif

#ifdef BEAGLEBONEBLACK
#define RESET_GPIO_PORT "30"
#define RESET_GPIO_DERICTION "/sys/class/gpio/gpio30/direction"
#define RESET_GPIO_VALUE "/sys/class/gpio/gpio30/value"
#endif

#ifdef ODROID_XU4
#define RESET_GPIO_PORT "18"
#define RESET_GPIO_DERICTION "/sys/class/gpio/gpio18/direction"
#define RESET_GPIO_VALUE "/sys/class/gpio/gpio18/value"
#endif

#ifdef SLSERVER_WIN32
#include <winsock2.h>
#include <windows.h>
#endif
#endif
