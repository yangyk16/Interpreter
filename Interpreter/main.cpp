// Interpreter.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "interpreter.h"
#include <stdlib.h>
#include <stdio.h>

extern c_interpreter myinterpreter;

int _tmain(int argc, _TCHAR* argv[])
{
	long long a = 0xbf85bfd5ad5de68d;
	printf("%f", *(double*)(&a));
	myinterpreter.run_interpreter();
	//getchar();
	return 0;
}