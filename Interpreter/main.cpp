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
	char *cmdname;	
	char *issue;
	cctool cmdptr;
} tool_cmd;

extern tty stdio;
extern file fileio;
int ycc(int argc, char **argv)
{
	int ret;
	char ch;
	int run_flag = 0;
	int link_flag = LINK_ADDR;
	int extra_flag = 0;
	char *output_file_name = NULL;
	char *fun_file_name = NULL;
	while((ch = kgetopt(argc, (char**)argv, "iceo:rg")) != -1) {
		switch(ch) {
		case 'i':
			link_flag = LINK_ADDR;
			break;
		case 'c':
			link_flag = LINK_STRNO;
			break;
		case 'e':
			link_flag = LINK_NUMBER;
			break;
		case 'o':
			output_file_name = optarg;
			break;
		case 'r':
			run_flag = 1;
			break;
		case 'g':
			extra_flag |= EXTRA_FLAG_DEBUG;
			break;
		}
		debug("opt=%c,arg=%s\n", ch, optarg);
	}
	if(run_flag) {
		myinterpreter.init(&stdio, RTL_FLAG_IMMEDIATELY);
		myinterpreter.load_ofile(argv[optind], 0);
		myinterpreter.run_main();
		myinterpreter.run_interpreter();
	}
	switch(link_flag) {
		case LINK_ADDR:
			if(optind == argc) {
				myinterpreter.init(&stdio, RTL_FLAG_IMMEDIATELY);
			} else {
				myinterpreter.init(&fileio, RTL_FLAG_IMMEDIATELY);
				fileio.init(argv[optind]);
			}
			irq_interpreter.init(0, 1);
			myinterpreter.run_interpreter();
			break;
		case LINK_STRNO:
		{
			int file_count = argc - optind;
			for(int i=0; i<file_count; i++) {
				ret = fileio.init(argv[optind + i]);
				if(ret)
					return ERROR_FILE;
				myinterpreter.init(&fileio, RTL_FLAG_DELAY);
				tip("compiling %s...", argv[optind + i]);
				myinterpreter.run_interpreter();
				int len = kstrlen(argv[optind + i]);
				argv[optind + i][len - 1] = 'o';
				ret = myinterpreter.tlink(LINK_STRNO);
				//compile(argv[optind + i], EXPORT_FLAG_LINK);
				myinterpreter.write_ofile(argv[optind + i], EXPORT_FLAG_LINK, extra_flag);//TODO: -o参数后的文件名和.c改.o规则冲突情况处理
				tip("%s made success!\n", argv[optind + i]);
			}
			break;
		}
		case LINK_NUMBER:
		{
			int file_count = argc - optind;
			myinterpreter.init(&stdio, RTL_FLAG_IMMEDIATELY);
			for(int i=0; i<file_count; i++) {
				myinterpreter.load_ofile(argv[optind + i], 0);
			}
			ret = myinterpreter.tlink(LINK_NUMBER);
			//myinterpreter.run_interpreter();
			if(ret)
				return ret;
			if(!output_file_name)
				output_file_name = "a.elf";
			myinterpreter.write_ofile(output_file_name, LINK_NUMBER, extra_flag);
			break;
		}
	}
	return 0;
}

int gdb(int argc, char **argv)
{
	return 0;
}

int objdump(int argc, char **argv)
{
	return 0;
}

tool_cmd cmdset[] = {
	{"ycc", "", ycc},
	{"gdb", "", gdb},
	{"objdump", "", objdump},
};

#ifdef WIN32
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char *argv[])
#endif
{
	signal(SIGINT, ouch);
	for(int i=1; i<argc; i++)
		tch2ch((char*)argv[i]);
	global_init();
	for(int i=0; i<sizeof(cmdset)/sizeof(tool_cmd); i++) {
		if(!kstrcmp((char*)argv[1], cmdset[i].cmdname)) {
			cmdset[i].cmdptr(argc - 1, (char**)&argv[1]);
			break;
		}
	}
	error("no cmd.\n");
	getchar();
	return 0;
}
