#include "arduino_cmd.h"
#include <string.h>
#include <stdio.h>
#define PARSE_COMMAND 0
#define PARSE_ARGS    1
#define PARSE_STRING  2
#define PARSE_ERROR   3
#define PARSE_END     4

char arduino_cmd::argv[MAX_CMD_ARGS][MAX_CMD_ARGUMENT_LEN];
int arduino_cmd::args = 0;
char arduino_cmd::cmd[MAX_CMD_ARGUMENT_LEN];

ino_command arduino_cmd::inocmd[MAX_INOCMD];
int arduino_cmd::cmd_count = 0;
int arduino_cmd::parseState = PARSE_COMMAND;
int arduino_cmd::byteOffset = 0;

bool arduino_cmd::parse(const char c) {
	
	if(byteOffset >= MAX_CMD_ARGUMENT_LEN) {
		byteOffset = 0;
		parseState = PARSE_ERROR;
	} if(PARSE_COMMAND == parseState) {
			
		if(c != '(')
			cmd[byteOffset++] = c;
		else {
			cmd[byteOffset++] = 0;
			byteOffset = 0;
			args = 0;
			parseState = PARSE_ARGS;
		}
		
	}else if(PARSE_ARGS == parseState) {
		
		if(c == ',') {
			argv[args][byteOffset++] = 0;
			byteOffset = 0;
			args++;		
			if(args > MAX_CMD_ARGS) {
				parseState = PARSE_ERROR;
			}
		}else if(c == ')') {
			argv[args][byteOffset++] = 0;
			args++;
			if( argv[0][0] == 0)
				args = 0;
			parseState = PARSE_END;
			return true;
		}else if(c == '\"') {
			parseState = PARSE_STRING;
		}else if(c != ' '&&c != '\t') {
			argv[args][byteOffset++] = c;
		}
			
		
	}else if(PARSE_STRING == parseState) {
			
		if( c != '\"' ) 
			argv[args][byteOffset++] = c;
		else
			parseState = PARSE_ARGS;
			
	}else if(PARSE_ERROR == parseState) {
		
		if( c == ')') {
			args = 0;
			*cmd = 0;
			parseState = PARSE_END;
			return false;
		}
	}else {
		if(c != ' '&&c != '\t'&&c != '\r'&&c != '\n') {
			parseState = PARSE_COMMAND;
			byteOffset = 0;
			cmd[byteOffset++] = c;
		}
	}
	
	return false;
}

void arduino_cmd::parse_command(const char* p) {

	while(*p) {
		if(parse(*p) == true) {
			execute();
		}
		p++;
	}
};
int times = 0;
int arduino_cmd::add_command(const char* str,ino_execute exec,void* user) {
	if(cmd_count < MAX_INOCMD) {
		strcpy_s(inocmd[cmd_count].cmd,str);
		inocmd[cmd_count].exec = exec;
		inocmd[cmd_count].user = user;
		cmd_count++;
	}
	return cmd_count;
}

int arduino_cmd::execute() {
	
	ino_command* ino = NULL;
	int ret = -1;
	printf("times %d function:%s\n", times++, cmd );
	for (int i = 0; i < args; i++) {
		printf("argv %d:%s\n",i,argv[i]);
		if (times == 200)
		{
			printf("2000\n");
		}
	}
	
	for(int i = 0; i < cmd_count;i++) {
		if(!strcmp(cmd,inocmd[i].cmd)) {
			ino = &inocmd[i];
			break;
		}
	}
	if(ino != NULL && ino->exec != NULL)
		ret = (*ino->exec)(args,argv,ino->user);
	
	return ret;
}
