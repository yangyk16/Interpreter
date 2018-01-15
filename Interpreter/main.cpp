// Interpreter.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "interpreter.h"
#include <stdlib.h>
#include <stdio.h>
#include "kmalloc.h"
#include <iostream>
using namespace std;

extern c_interpreter myinterpreter;

int _tmain(int argc, _TCHAR* argv[])
{
	extern char* heapbase;
	int *a, *b, *c, *d;
	heapinit();
	cout<<"heapbase="<<(void*)heapbase<<endl;
	a = (int*)kmalloc(10);
	cout<<"t="<<a<<endl;
	b = (int*)kmalloc(49);
	cout<<"t="<<b<<endl;
	c = (int*)kmalloc(31);
	cout<<"t="<<c<<endl;	
	d = (int*)kmalloc(33);
	cout<<"t="<<d<<endl;
	krealloc(b, 15);
	c = (int*)kmalloc(16);
	cout<<"t="<<c<<endl;
	kfree(c);
	c = (int*)kmalloc(1);
	cout<<"t="<<c<<endl;	
	myinterpreter.run_interpreter();
	//getchar();
	return 0;
}