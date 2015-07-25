#ifndef _SLGETOPT_H_
#define _SLGETOPT_H_
#include "slconfig.h"

#ifdef SLSERVER_WIN32
#ifdef __cplusplus
extern "C" {
#endif
extern char* optarg;
extern int optind;
extern int opterr;
extern int optopt;
int getopt(int argc, char** argv, char* optstr);
#ifdef __cplusplus
}
#endif

#endif
#endif