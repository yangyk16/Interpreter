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

int _tmain(int argc, _TCHAR* argv[])
{
	signal(SIGINT, ouch);
	global_init();
	myinterpreter.run_interpreter();
	//getchar();
	return 0;
}