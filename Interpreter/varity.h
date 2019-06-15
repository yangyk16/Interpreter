#pragma once
#ifndef VARITY_H
#define VARITY_H

#include "config.h"
#include "type.h"
#include "data_struct.h"

#define BASIC_VARITY_TYPE_COUNT	15
#define TMP_VAIRTY_PREFIX	0x02 //STX
#define LINK_VARITY_PREFIX	0x03

#define PRODUCED_DECLARE	1
#define PRODUCED_ALL		3

#define VARITY_SCOPE_EXTERNAL	4
#define VARITY_SCOPE_TMP		2
#define VARITY_SCOPE_GLOBAL  	1
#define VARITY_SCOPE_LOCAL   	0

#define COMPLEX_ARRAY	1u
#define COMPLEX_PTR		2u
#define COMPLEX_ARG		3u
#define COMPLEX_COMPLEX	4u
#define COMPLEX_BASIC	5u

#define COMPLEX_TYPE_BIT	29
#define COMPLEX_DATA_BIT_MASK	(~(0xFFFFFFFFu >> COMPLEX_TYPE_BIT << COMPLEX_TYPE_BIT))
#define COMPLEX_TYPE_BIT_MASK	(~COMPLEX_DATA_BIT_MASK)

#define GET_COMPLEX_TYPE(x)		((uint)(x) >> COMPLEX_TYPE_BIT)
#define GET_COMPLEX_DATA(x)		((uint)(x) & COMPLEX_DATA_BIT_MASK)
#define SET_COMPLEX_TYPE(x)		((uint)(x) << COMPLEX_TYPE_BIT)

#define BASIC_TYPE_SET(x)		((x) | COMPLEX_BASIC << COMPLEX_TYPE_BIT)

#if PLATFORM_WORD_LEN == 4
#define LONG 6u
#define U_INT 7u
#define U_LONG 8u
#define LONG_LONG 9u
#elif PLATFORM_WORD_LEN == 8
#define U_INT 6u
#define LONG 7u
#define LONG_LONG 8u
#define U_LONG 9u
#endif
#define STRUCT	14u
#define VOID 13u
#define DOUBLE 12u
#define FLOAT 11u
#define U_LONG_LONG 10u
#define INT 5u
#define U_SHORT 4u
#define SHORT 3u
#define U_CHAR 2u
#define CHAR 1u
#define COMPLEX	0u
#define PTR 16u
#define ARRAY 17u
#define PLATFORM_TYPE LONG
typedef unsigned long PLATFORM_WORD;

class varity_info:public element {
protected:
	uint size;//TODO: remove size
	PLATFORM_WORD* comlex_info_ptr;
	void*	content_ptr;
	int complex_arg_count;
public:
	inline uint get_size(void){return this->size;}
	int get_type(void);
	void init_varity(char*, uint, int, PLATFORM_WORD*);
	void arg_init(char*, uint, int, PLATFORM_WORD*, void*);
	void config_complex_info(int, PLATFORM_WORD*);
	varity_info();
	varity_info(char*, int, uint);
	//operator varity_info();
	int apply_space(void);
	int get_element_size(void);
	int get_first_order_sub_struct_size(void);
	void *get_content_ptr(void){return content_ptr;}
	PLATFORM_WORD *&get_complex_ptr(void){return this->comlex_info_ptr;}
	int  get_complex_arg_count(void) {return this->complex_arg_count;}
	void set_complex_arg_count(int n) {this->complex_arg_count = n;}
	void set_size(uint size) {this->size = size;}
	void set_content_ptr(void* addr){this->content_ptr = addr;}
	void set_type(int type);
	void set_name(char *name_ptr) {this->name = name_ptr;}
	void reset(void);
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
	varity_info* find(char*);
	varity_info* vfind(char *name, int &scope);
	int declare(int scope_flag, char* name, uint size, int, PLATFORM_WORD*);
	int destroy_local_varity_cur_depth(void);
	int destroy_local_varity(void);
	int undeclare();
};

class struct_define;

int get_varity_size(int basic_type, PLATFORM_WORD *complex_info = 0, int complex_arg_count = 0);
int array_to_ptr(PLATFORM_WORD *&complex_info, int complex_arg_count);
void dec_varity_ref(varity_info *varity_ptr, bool destroy_flag);
void inc_varity_ref(varity_info *varity_ptr);
void *get_basic_info(int basic_type, void *info_ptr, struct_define *struct_define_ptr);
int get_element_size(int complex_arg_count, PLATFORM_WORD *comlex_info_ptr);
int destroy_varity_stack(stack *stack_ptr);
#if DEBUG_EN
void print_varity(int format, int complex_count, PLATFORM_WORD *complex_ptr, void *content_ptr);
#endif

extern const char type_key[15][19];
extern const char sizeof_type[15];
extern const char type_len[15];
extern PLATFORM_WORD basic_type_info[15][4];
#endif
