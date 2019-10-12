#pragma once

#ifndef HAL_H
#define HAL_H

#include "config.h"
typedef char *(*code_fptr)(char*, int);
class terminal{
	int echo_en;
public:
	terminal(int echo_en) {this->echo_en = echo_en;}
	int readline(char* str, char code_ch, code_fptr callback);
	int t_puts(char *str) {for(int i=0; str[i]; i++) this->t_putc(str[i]); return 0;}
	virtual int t_putc(char) = 0;
	virtual int t_getc(char *ch) = 0;
	virtual void dispose(void) = 0;
};

#if TTY_TYPE == 0 
class tty: public terminal {
public:
	tty(int echo_en): terminal(echo_en) {}
	//virtual int readline(char* str, char code_ch, code_fptr callback);
	virtual int t_putc(char);
	virtual int t_getc(char *ch);
	virtual void dispose(void){};
};
#elif TTY_TYPE == 1
class uart: public terminal {
public:
	uart(int echo_en): terminal(echo_en) {}
	//virtual int readline(char* str, char code_ch, code_fptr callback);
	virtual int t_putc(char);
	virtual int t_getc(char *ch);
	virtual void dispose(void){};
};
#endif
class file: public terminal {
	void *file_ptr;
public:
	file(int echo_en): terminal(echo_en) {}
	//virtual int readline(char* str, char code_ch, code_fptr callback);
	virtual int t_putc(char ch){return 0;}
	virtual int t_getc(char *ch);
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
