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
}

int _tmain(int argc, _TCHAR* argv[])
{
	extern char* heapbase;
	int *a, *b, *c, *d, *e, *f, *g, *h;
	signal(SIGINT, ouch);
	global_init();
	cout<<"heapbase="<<(void*)heapbase<<endl;
	a = (int*)kmalloc(192);
	cout<<"a="<<a<<endl;
	b = (int*)kmalloc(2);
	cout<<"b="<<b<<endl;	
	c = (int*)kmalloc(2);
	cout<<"c="<<c<<endl;
	d = (int*)kmalloc(4096);
	cout<<"d="<<d<<endl;
	e = (int*)kmalloc(512);
	cout<<"e="<<e<<endl;
	f = (int*)kmalloc(512);
	cout<<"f="<<f<<endl;
	krealloc(d, 74);
	kfree(f);
	krealloc(e,7);
	kfree(b);
	g = (int*)kmalloc(2);
	cout<<"g="<<g<<endl;
	h = (int*)kmalloc(2);
	cout<<"h="<<h<<endl;
	myinterpreter.run_interpreter();
	//getchar();
	return 0;
}