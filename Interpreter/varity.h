#pragma once
#ifndef VARITY_H
#define VARITY_H

#include "type.h"
#include "data_struct.h"

#define BASIC_VARITY_TYPE_COUNT	15
#define TMP_VAIRTY_PREFIX	0x02 //STX
#define LINK_VARITY_PREFIX	0x03

#define PRODUCED_DECLARE	1
#define PRODUCED_ANALIES	2
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

#define GET_COMPLEX_TYPE(x)		(((x) & COMPLEX_TYPE_BIT_MASK) >> COMPLEX_TYPE_BIT)
#define GET_COMPLEX_DATA(x)		((x) & COMPLEX_DATA_BIT_MASK)

#if PLATFORM_WORD_LEN == 4
#define U_INT 8
#define LONG 9
#elif PLATFORM_WORD_LEN == 8
#define LONG 8
#define U_INT 9
#endif
#define COMPLEX	0
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

class varity_attribute: public element {
protected:
	char type;
	char attribute;
	uint size;
	void*	comlex_info_ptr;
	int complex_arg_count;
public:
	inline uint get_size(void){return this->size;}
	inline int get_type(void) {return this->type;}
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
	void config_complex_info(int, void*);
	void clear_attribute(char);
	varity_info();
	varity_info(char*, int, uint);
	varity_info& operator=(const varity_info&);
	varity_info& operator=(char);
	varity_info& operator=(unsigned char);
	varity_info& operator=(short);
	varity_info& operator=(unsigned short);
	varity_info& operator=(const int&);
	varity_info& operator=(unsigned int);
	varity_info& operator=(long long);
	varity_info& operator=(unsigned long long);
	varity_info& operator=(float);
	varity_info& operator=(double);
	varity_info& operator++(void);//Ç°ÖÃ++
	varity_info& operator++(int);//ºóÖÃ++
	varity_info& operator--(void);
	varity_info& operator--(int);
	friend varity_info& operator~(varity_info&);
	friend varity_info& operator!(varity_info&);
	friend varity_info& operator+(varity_info&);
	friend varity_info& operator-(varity_info&);
	friend varity_info& operator*(varity_info&, varity_info&);
	friend varity_info& operator/(varity_info&, varity_info&);
	friend varity_info& operator%(varity_info&, varity_info&);
	friend varity_info& operator+(varity_info&, varity_info&);
	friend varity_info& operator-(varity_info&, varity_info&);
	friend varity_info& operator>>(varity_info&, varity_info&);
	friend varity_info& operator<<(varity_info&, varity_info&);
	friend varity_info& operator>(varity_info&, varity_info&);
	friend varity_info& operator<(varity_info&, varity_info&);
	friend varity_info& operator>=(varity_info&, varity_info&);
	friend varity_info& operator<=(varity_info&, varity_info&);
	friend varity_info& operator==(varity_info&, varity_info&);
	friend varity_info& operator!=(varity_info&, varity_info&);
	operator varity_info();
	void create_from_c_varity(void*, int);
	int apply_space(void);
	int struct_apply(void);
	int get_element_size(void);
	void* get_content_ptr(void){return content_ptr;}
	void* get_complex_ptr(void){return this->comlex_info_ptr;}
	int   get_complex_arg_count(void) {return this->complex_arg_count;}
	void* get_element_ptr(int);
	void set_complex_arg_count(int n) {this->complex_arg_count = n;}
	void set_to_single(int);
	void set_size(uint size) {this->size = size;}
	void set_content_ptr(void* addr){this->content_ptr = addr;}
	void set_type(int type){this->type = type;}
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
	varity(stack*, indexed_stack*, stack*);
	stack* global_varity_stack;
	stack* analysis_varity_stack;
	indexed_stack* local_varity_stack;
	varity_info* find(char*, int);
	varity_info* vfind(char *name, int &scope);
	int declare(int scope_flag, char* name, char type, uint size, char = 0);
	int declare_analysis_varity(char type, uint size, char*, varity_info**, char = 0);
	int destroy_analysis_varity(void);
	int destroy_analysis_varity(int);
	int destroy_local_varity_cur_depth(void);
	int destroy_local_varity(void);
	int undeclare();
};

void* vmalloc(unsigned int size);
int get_varity_size(int basic_type, uint *complex_info = 0, int complex_arg_count = 0);

extern const char type_key[15][19];
extern const char sizeof_type[15];
extern const char type_len[15];
#endif
