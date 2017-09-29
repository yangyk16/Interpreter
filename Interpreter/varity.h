#pragma once
#ifndef VARITY_H
#define VARITY_H

#include "type.h"
#include "data_struct.h"

#define BASIC_VARITY_TYPE_COUNT	15
#define TMP_VAIRTY_PREFIX 0x02 //STX
#define PRODUCED_DECLARE	1
#define PRODUCED_ANALIES	2
#define PRODUCED_ALL		3

#if PLATFORM_WORD_LEN == 4
#define U_INT 8
#define LONG 9
#elif PLATFORM_WORD_LEN == 8
#define LONG 8
#define U_INT 9
#endif
#define EMPTY	0
#define STRUCT	1
#define VOID 2
#define DOUBLE 3
#define FLOAT 4
#define U_LONG_LONG 5
#define LONG_LONG 6
#define U_LONG 7
#define INT 10
#define U_SHORT 11
#define SHORT 12
#define U_CHAR 13
#define CHAR 14

class varity_info:public element {
protected:
	char 	type;
	char	attribute;
	uint 	size;
	void*	content_ptr;
public:
	static bool en_echo;
	static void init_varity(void*, char*, int, uint);
	varity_info();
	varity_info(char*, int, uint);
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
	friend varity_info& operator+(varity_info&, varity_info&);
	friend varity_info& operator>(varity_info&, varity_info&);
	friend varity_info& operator<(varity_info&, varity_info&);
	operator varity_info();
	void create_from_c_varity(void*, int);
	int apply_space(void);
	void* get_content_ptr(void){return content_ptr;}
	uint get_size(void){return size;}
	void reset(void);
	void echo(void);
	void convert(void*,int);
	void set_echo(bool enable) {en_echo = enable;}
	inline int get_type(void) {return this->type;}
	int is_non_zero(void);
	~varity_info(){this->reset();}
};

class varity {
	data_struct* frame_varity_struct;
	uint current_stack_depth;
	int cur_analysis_varity_count;
public:
	varity(stack*, indexed_stack*, stack*);
	stack* global_varity_stack;
	stack* analysis_varity_stack;
	indexed_stack* local_varity_stack;
	varity_info* find(char*, int);
	int declare(int scope_flag, char* name, char type, uint size);
	int declare_analysis_varity(int type, uint size, char*, varity_info**);
	int destroy_analysis_varity(void);
	int undeclare();
};

void* vmalloc(unsigned int size);

extern const char type_key[13][19];
extern const char sizeof_type[13];
#endif
