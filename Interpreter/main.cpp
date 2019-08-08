// Interpreter.cpp : 定义控制台应用程序的入口点。
//

#ifdef WIN32
#include "stdafx.h"
#endif
#include "interpreter.h"
#include <stdlib.h>
#include <stdio.h>
#include "kmalloc.h"
#include <iostream>
#include <signal.h>
#include "getopt.h"
#include "error.h"
#include "toolchain.h"

using namespace std;

extern c_interpreter myinterpreter;
extern c_interpreter irq_interpreter;

void ouch(int sig)
{
	myinterpreter.set_break_flag(1);
	myinterpreter.print_call_stack();
}

void tch2ch(char *src)
{
	char *dest = src;
	for(; *src;) {
		*dest++ = *src;
		if(*(src + 1))
			*dest++ = *(src + 1);
		src += 2;
	}
	*dest = 0;
}

typedef int (*cctool)(int argc, char **argv);
typedef struct tool_cmd_s {
	const char *cmdname;	
	const char *issue;
	cctool cmdptr;
} tool_cmd;

tool_cmd cmdset[] = {
	{"cc", "", cc},
	{"db", "", db},
	{"objdump", "", objdump},
};

#ifdef WIN32
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char *argv[])
#endif
{
	signal(SIGINT, ouch);
#ifdef WIN32
	for(int i=1; i<argc; i++)
		tch2ch((char*)argv[i]);
#endif
	global_init();
	for(int i=0; i<sizeof(cmdset)/sizeof(tool_cmd); i++) {
		if(!kstrcmp((char*)argv[1], cmdset[i].cmdname)) {
			cmdset[i].cmdptr(argc - 1, (char**)&argv[1]);
			break;
		}
	}
	global_dispose();
	error("no cmd.\n");
	getchar();
	return 0;
}
