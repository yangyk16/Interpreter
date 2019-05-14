// Interpreter.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "interpreter.h"
#include <stdlib.h>
#include <stdio.h>
#include "kmalloc.h"
#include <iostream>
#include <signal.h>
using namespace std;

extern c_interpreter myinterpreter;

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

extern tty stdio;
extern file fileio;
int _tmain(int argc, _TCHAR* argv[])
{
	signal(SIGINT, ouch);
	for(int i=1; i<argc; i++)
		tch2ch((char*)argv[i]);
	global_init();
	if(argc == 1)
		myinterpreter.init(&stdio, 1);
	else {
		myinterpreter.init(&fileio, 1);
		fileio.init((char*)argv[1]);
	}
	myinterpreter.run_interpreter();
	getchar();
	return 0;
}