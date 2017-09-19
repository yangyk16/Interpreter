#pragma once
#ifndef VARITY_H
#define VARITY_H

#include "type.h"
#include "data_struct.h"

#if PLATFORM_WORD_LEN == 4
#define U_INT 6
#define LONG 7
#elif PLATFORM_WORD_LEN == 8
#define LONG 6
#define U_INT 7
#endif
#define VOID 0
#define DOUBLE 1
#define FLOAT 2
#define U_LONG_LONG 3
#define LONG_LONG 4
#define U_LONG 5
#define INT 8
#define U_SHORT 9
#define SHORT 10
#define U_CHAR 11
#define CHAR 12

#define VARITY_NOSPACE		1
#define VARITY_DUPLICATE	2
#define VARITY_COUNT_MAX	3

class varity_info:public element {
protected:
	int	 	type;
	uint 	size;
	void*	content_ptr; 
public:
	varity_info(char*, int, uint);
	varity_info();
	varity_info(char);
	varity_info(unsigned char);
	varity_info(short);
	varity_info(unsigned short);
	varity_info(int);
	varity_info(unsigned int);
	varity_info(long long);
	varity_info(unsigned long long);
	varity_info(float);
	varity_info(double);
	friend varity_info operator+(varity_info&, varity_info&);
	int apply_space(void);
	void* get_content_ptr(void){return content_ptr;}
	uint get_size(void){return size;}
};

class varity {
	data_struct* frame_varity_struct;
	uint current_stack_depth;
public:
	varity(stack*, indexed_stack*);
	stack* global_varity_stack;
	indexed_stack* local_varity_stack;
	int declare(bool global_flag, char* name, int type, uint size);
	int undeclare();
};

extern const char type_key[13][19];
extern const char sizeof_type[13];
#endif
