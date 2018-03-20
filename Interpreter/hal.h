#pragma once

#ifndef HAL_H
#define HAL_H

#include "config.h"

class terminal{
public:
	virtual int readline(char*) = 0;
	virtual int puts(char*) = 0;
};

#if TTY_TYPE == 0 
class tty: public terminal {
public:
	virtual int readline(char*);
	virtual int puts(char*);
};
#elif TTY_TYPE == 1
class uart: public terminal {
public:
	virtual int readline(char*);
	virtual int puts(char*);
};
#endif

int kfputs(char *str);
int hard_fault_check(int addr);
#endif
