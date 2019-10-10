#pragma once

#ifndef HAL_H
#define HAL_H

#include "config.h"

typedef char *(*code_fptr)(char*, int);
class terminal{
public:
	virtual int readline(char* str, char code_ch, code_fptr callback) = 0;
	virtual int puts(char*) = 0;
	virtual void dispose(void) = 0;
};

#if TTY_TYPE == 0 
class tty: public terminal {
public:
	virtual int readline(char* str, char code_ch, code_fptr callback);
	virtual int puts(char*);
	virtual void dispose(void){};
};
#elif TTY_TYPE == 1
class uart: public terminal {
public:
	virtual int readline(char* str, char code_ch, code_fptr callback);
	virtual int puts(char*);
	virtual void dispose(void){};
};
#endif
class file: public terminal {
	void *file_ptr;
public:
	virtual int readline(char* str, char code_ch, code_fptr callback);
	virtual int puts(char*){return 0;}
	virtual void dispose(void);
	int init(char*);
};

int kfputs(char *str);
void *kfopen(const char *filename, const char *mode);
int kfclose(void*);
unsigned int kfread(void *buffer, unsigned int size, unsigned int count, void *fileptr);
unsigned int kfwrite(void *buffer, unsigned int size, unsigned int count, void *fileptr);
int hard_fault_check(int addr);
#endif
