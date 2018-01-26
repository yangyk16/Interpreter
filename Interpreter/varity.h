#pragma once
#ifndef VARITY_H
#define VARITY_H

#include "type.h"
#include "data_struct.h"

#define BASIC_VARITY_TYPE_COUNT	15
#define TMP_VAIRTY_PREFIX	0x02 //STX
#define LINK_VARITY_PREFIX	0x03

#define PRODUCED_DECLARE	1
#define PRODUCED_ALL		3

#define VARITY_SCOPE_GLOBAL  	0
#define VARITY_SCOPE_LOCAL   	1
#define VARITY_SCOPE_ANALYSIS	2

#define ATTRIBUTE_NORMAL		0
#define ATTRIBUTE_LINK			(1 << 0)
#define ATTRIBUTE_TYPE_UNFIXED	(1 << 1)
#define ATTRIBUTE_ARRAY			(1 << 2)
#define ATTRIBUTE_COMPLEX		(1 << 3)
#define ATTRIBUTE_RIGHT_VALUE	(1 << 4)
#define ATTRIBUTE_REFERENCE		(1 << 5)

#define COMPLEX_ARRAY	1
#define COMPLEX_PTR		2
#define COMPLEX_ARG		3
#define COMPLEX_COMPLEX	4
#define COMPLEX_BASIC	5

#define COMPLEX_TYPE_BIT	29
#define COMPLEX_DATA_BIT_MASK	(~(0xFFFFFFFFu >> COMPLEX_TYPE_BIT << COMPLEX_TYPE_BIT))
#define COMPLEX_TYPE_BIT_MASK	(~COMPLEX_DATA_BIT_MASK)

#define GET_COMPLEX_TYPE(x)		((uint)(x) >> COMPLEX_TYPE_BIT)
#define GET_COMPLEX_DATA(x)		((uint)(x) & COMPLEX_DATA_BIT_MASK)
#define SET_COMPLEX_TYPE(x)		((uint)(x) << COMPLEX_TYPE_BIT)

#define BASIC_TYPE_SET(x)		((x) | COMPLEX_BASIC << COMPLEX_TYPE_BIT)

#if PLATFORM_WORD_LEN == 4
#define LONG 6
#define U_INT 7
#define U_LONG 8
#define LONG_LONG 9
#elif PLATFORM_WORD_LEN == 8
#define U_INT 6
#define LONG 7
#define LONG_LONG 8
#define U_LONG 9
#endif
#define STRUCT	14
#define VOID 13
#define DOUBLE 12
#define FLOAT 11
#define U_LONG_LONG 10
#define INT 5
#define U_SHORT 4
#define SHORT 3
#define U_CHAR 2
#define CHAR 1
#define COMPLEX	0
#define PTR 16
#define ARRAY 17
#define PLATFORM_TYPE LONG
typedef unsigned long PLATFORM_WORD;

class varity_attribute: public element {
protected:
	char type;
	char attribute;
	uint size;
	PLATFORM_WORD* comlex_info_ptr;
public:
	int complex_arg_count;
	inline uint get_size(void){return this->size;}
	int get_type(void);
	static void init(void*, char*, char, char, uint);
};

class varity_info:public varity_attribute {
protected:
	void*	content_ptr;
public:
	static bool en_echo;
	static void init_varity(void*, char*, char, uint);
	void arg_init(char*, char, uint, void*);
	void config_varity(char, void* = 0);
	void config_complex_info(int, PLATFORM_WORD*);
	void clear_attribute(char);
	varity_info();
	varity_info(char*, int, uint);
	operator varity_info();
	int apply_space(void);
	int struct_apply(void);
	int get_element_size(void);
	int get_first_order_sub_struct_size(void);
	void *get_content_ptr(void){return content_ptr;}
	PLATFORM_WORD *&get_complex_ptr(void){return this->comlex_info_ptr;}
	int  get_complex_arg_count(void) {return this->complex_arg_count;}
	void *get_element_ptr(int);
	void set_complex_arg_count(int n) {this->complex_arg_count = n;}
	void set_to_single(int);
	void set_size(uint size) {this->size = size;}
	void set_content_ptr(void* addr){this->content_ptr = addr;}
	void set_type(int type);
	char get_attribute(void){return this->attribute;}
	void reset(void);
	void echo(void);
	void convert(void*,int);
	void set_echo(bool enable) {en_echo = enable;}
	int is_non_zero(void);
	~varity_info(){this->reset();}
};

class varity {
	data_struct* frame_varity_struct;
	uint current_stack_depth;
	int cur_analysis_varity_count;
public:
	void init(stack*, indexed_stack*);
	stack* global_varity_stack;
	indexed_stack* local_varity_stack;
	varity_info* find(char*, int);
	varity_info* vfind(char *name, int &scope);
	int declare(int scope_flag, char* name, char type, uint size, int, PLATFORM_WORD*);
	int destroy_local_varity_cur_depth(void);
	int destroy_local_varity(void);
	int undeclare();
};

class struct_define;

void *vmalloc(unsigned int size);
void vfree(void*);
void* vrealloc(void* addr, unsigned int size);
int get_varity_size(int basic_type, PLATFORM_WORD *complex_info = 0, int complex_arg_count = 0);
int array_to_ptr(PLATFORM_WORD *&complex_info, int complex_arg_count);
void dec_varity_ref(varity_info *varity_ptr, bool destroy_flag);
void inc_varity_ref(varity_info *varity_ptr);
void *get_basic_info(int basic_type, void *info_ptr, struct_define *struct_define_ptr);
int destroy_varity_stack(stack *stack_ptr);

extern const char type_key[15][19];
extern const char sizeof_type[15];
extern const char type_len[15];
extern PLATFORM_WORD basic_type_info[15][4];
#endif
