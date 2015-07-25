#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getopt.h"

#ifdef SLSERVER_WIN32
static const char* ID = "$Id: getopt.c,v 1.2 2003/10/26 03:10:20 vindaci Exp $";

char* optarg = NULL;
int optind = 0;
int opterr = 1;
int optopt = '?';

static char** prev_argv = NULL;
static int prev_argc = 0;
static int argv_index = 0;
static int argv_index2 = 0;
static int opt_offset = 0;
static int dashdash = 0;
static int nonopt = 0;

static void increment_index() {

	if(argv_index < argv_index2) {
		while(prev_argv[++argv_index] && prev_argv[argv_index][0] != '-'
		        && argv_index < argv_index2+1);
	} else argv_index++;
	opt_offset = 1;
}

static int permute_argv_once() {

	if(argv_index + nonopt >= prev_argc) return 1;

	else {
		char* tmp = prev_argv[argv_index];


		memmove(&prev_argv[argv_index], &prev_argv[argv_index+1],
		        sizeof(char**) * (prev_argc - argv_index - 1));
		prev_argv[prev_argc - 1] = tmp;

		nonopt++;
		return 0;
	}
}

int getopt(int argc, char** argv, char* optstr) {
	int c = 0;

	if(prev_argv != argv || prev_argc != argc) {

		prev_argv = argv;
		prev_argc = argc;
		argv_index = 1;
		argv_index2 = 1;
		opt_offset = 1;
		dashdash = 0;
		nonopt = 0;
	}

getopt_top:
	optarg = NULL;
	
	if(argv[argv_index] && !strcmp(argv[argv_index], "--")) {
		dashdash = 1;
		increment_index();
	}

	if(argv[argv_index] == NULL) {
		c = -1;
	} else if(dashdash || argv[argv_index][0] != '-' 
			|| !strcmp(argv[argv_index], "-")) {

		if(optstr[0] == '-') {
			c = 1;
			optarg = argv[argv_index];
			increment_index();
		} else if(optstr[0] == '+') {
			c = -1;
			nonopt = argc - argv_index;
		} else {
			if(!permute_argv_once()) goto getopt_top;
			else c = -1;
		}
	} else {
		char* opt_ptr = NULL;
		c = argv[argv_index][opt_offset++];

		if(optstr[0] == '-') opt_ptr = strchr(optstr+1, c);
		else opt_ptr = strchr(optstr, c);

		if(!opt_ptr) {
			if(opterr) {
				fprintf(stderr, "%s: invalid option -- %c\n", argv[0], c);
			}

			optopt = c;
			c = '?';

			increment_index();
		} else if(opt_ptr[1] == ':') {

			if(argv[argv_index][opt_offset] != '\0') {
				optarg = &argv[argv_index][opt_offset];
				increment_index();
			} else if(opt_ptr[2] != ':') {

				if(argv_index2 < argv_index) argv_index2 = argv_index;
				while(argv[++argv_index2] && argv[argv_index2][0] == '-');
				optarg = argv[argv_index2];

				if(argv_index2 + nonopt >= prev_argc) optarg = NULL;
				increment_index();
			} else {
				increment_index();
			}
			if(optarg == NULL && opt_ptr[2] != ':') {
				optopt = c;
				c = '?';

				if(opterr) {
					fprintf(stderr,"%s: option requires an argument -- %c\n",
					        argv[0], optopt);
				}
			}
		} else {

			if(argv[argv_index][opt_offset] == '\0') {
				increment_index();
			}
		}
	}


	if(c == -1) {
		optind = argc - nonopt;
	} else {
		optind = argv_index;
	}

	return c;
}
#endif

