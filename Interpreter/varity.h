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
	static bool en_echo;
	varity_info(char*, int, uint);
	varity_info();
	varity_info& operator=(const varity_info&);
	varity_info& operator=(char);
	varity_info& operator=(unsigned char);
	varity_info& operator=(short);
	varity_info& operator=(unsigned short);
	varity_info& operator=(int);
	varity_info& operator=(unsigned int);
	varity_info& operator=(long long);
	varity_info& operator=(unsigned long long);
	varity_info& operator=(float);
	varity_info& operator=(double);
	void create_from_c_varity(void*, int);
	friend varity_info operator+(varity_info&, varity_info&);
	int apply_space(void);
	void* get_content_ptr(void){return content_ptr;}
	uint get_size(void){return size;}
	void echo(void);
	void convert(void*,int);
	void set_echo(bool enable) {en_echo = enable;}
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

void* vmalloc(unsigned int size);

extern const char type_key[13][19];
extern const char sizeof_type[13];
#endif
