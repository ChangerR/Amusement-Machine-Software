#ifndef __SL_ARDUINO_CMD_H
#define __SL_ARDUINO_CMD_H
#include "slconfig.h"

#define MAX_CMD_ARGUMENT_LEN 128
#define MAX_CMD_ARGS 16
#define MAX_INOCMD 128
typedef int (*ino_execute)(int,char (*)[MAX_CMD_ARGUMENT_LEN],void*);

struct ino_command {
	char cmd[32];
	ino_execute exec;
	void* user;
};

class arduino_cmd {
public:
	arduino_cmd(){};
	virtual ~arduino_cmd();
	
	static bool parse(const char c);
	static void parse_command(const char* );
	static int execute();
	static int add_command(const char* ,ino_execute,void*);
	
	static ino_command inocmd[];
	static int cmd_count;
public:
	static char argv[][MAX_CMD_ARGUMENT_LEN];
	static int args;
	static char cmd[MAX_CMD_ARGUMENT_LEN];
	static int parseState;
	static int byteOffset;
};

#endif