// Interpreter.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "interpreter.h"


extern c_interpreter myinterpreter;

int _tmain(int argc, _TCHAR* argv[])
{
	varity_info a;
	a = 5;
	myinterpreter.run_interpreter();
	//getchar();
	return 0;
}

