// Interpreter.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "interpreter.h"
#include <stdlib.h>
#include <stdio.h>

extern c_interpreter myinterpreter;

int _tmain(int argc, _TCHAR* argv[])
{
	myinterpreter.run_interpreter();
	//getchar();
	return 0;
}