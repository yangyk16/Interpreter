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
class file: public terminal {
	void *file_ptr;
public:
	virtual int readline(char*);
	virtual int puts(char*){return 0;}
	int init(char*);
};

int kfputs(char *str);
void *kfopen(char *filename, char *mode);
int kfclose(void*);
unsigned int kfread(void *buffer, unsigned int size, unsigned int count, void *fileptr);
unsigned int kfwrite(void *buffer, unsigned int size, unsigned int count, void *fileptr);
int hard_fault_check(int addr);
#endif
