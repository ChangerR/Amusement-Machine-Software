#include "arduino_cmd.h"
#include <string.h>

#define PARSE_COMMAND 0
#define PARSE_ARGS    1
#define PARSE_STRING  2
#define PARSE_ERROR   3
#define PARSE_END     4

char arduino_cmd::argument[MAX_CMD_ARGS][MAX_CMD_ARGUMENT_LEN];
int arduino_cmd::args = 0;
char arduino_cmd::cmd[MAX_CMD_ARGUMENT_LEN];

ino_command arduino_cmd::inocmd[MAX_INOCMD];
int arduino_cmd::cmd_count = 0;

bool arduino_cmd::parse_command(const char* p) {
	int parseState = PARSE_COMMAND;
	int offset = 0;
	args = 0;
	while(*p) {
		if(offset >= MAX_CMD_ARGUMENT_LEN)
			parseState = PARSE_ERROR;
		
		switch(parseState) {
		case PARSE_COMMAND:
		{	
			if(*p == '(') {
				cmd[offset++] = 0;
				parseState = PARSE_ARGS;
				offset = 0;
			}else 
				cmd[offset++] = *p;			
		}
			break;	
		case PARSE_ARGS:
		{
			if(*p == ',') {				
				argument[args][offset] = 0;
				args++;
				offset = 0;
				if(args >= MAX_CMD_ARGS)
					parseState = PARSE_ERROR;
			} else if(*p == ')') {
				argument[args][offset] = 0;
				args++;
				parseState = PARSE_END;
			} else if(*p == '\"') {
				parseState = PARSE_STRING;
			} else if(*p != ' ') {
				argument[args][offset++] = *p;
			}
		}
			break;
		case PARSE_STRING:
		{
			if(*p == '\"')
				parseState = PARSE_ARGS;
			else
				argument[args][offset++] = *p;
		}
			break;
		default:
			break;
		}	
		if(parseState == PARSE_END || parseState == PARSE_ERROR)
			break;
		p++;
	}
	
	if(parseState == PARSE_END)
		return true;
	else
		return false;
};

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
	
	for(int i = 0; i < cmd_count;i++) {
		if(!strcmp(cmd,inocmd[i].cmd)) {
			ino = &inocmd[i];
			break;
		}
	}
	if(ino != NULL && ino->exec != NULL)
		ret = (*ino->exec)(args,argument,ino->user);
	
	return ret;
}
