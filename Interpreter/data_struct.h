#pragma once
#ifndef DATA_STRUCTURE
#define DATA_STRUCTURE

#include "type.h"
#include "config.h"

class element {
protected:
	char*	name;
public:
	inline char* get_name(void) {return name;}
};

class data_struct {
protected:
	uint count;
	uint element_size;
	uint length;
	void* bottom_addr;
public:
	inline void* get_base_addr(void) {return this->bottom_addr;}
	inline int is_full(void) {return length==count?1:0;}
	inline uint get_count(void) {return this->count;}
};

class round_queue: public data_struct {
	uint wptr;
	uint rptr;
public:
	friend class c_interpreter;
	round_queue();
	round_queue(uint, void*);
	void init(uint len);
	inline void set_base(void* addr) {this->bottom_addr = addr;}
	inline void set_length(uint len) {this->length = len;}
	inline void set_element_size(uint len) {this->element_size = len;}
	int write(void*, uint);
	int read(void*, uint);
	int readline(char*);
	char* readline(int&);
};

class stack: public data_struct {
protected:
	uint top;
public:
	void push(void*);
	void push(void) {this->top += this->element_size; this->count++;}
	void* pop(void);
	virtual void* find(char*);
	inline void set_base(void* addr) {this->bottom_addr = addr;}
	//inline void set_length(uint len) {this->length = len;}
	stack();
	stack(int esize, void* base_addr, int capacity);
	void init(int, void*, int);
	void* get_current_ptr(void) {return (char*)this->bottom_addr + top;}
	void* get_lastest_element(void) {return (char*)get_current_ptr() - this->element_size;}
	void* visit_element_by_index(int);
};

class indexed_stack: public stack {
	int current_depth;
	int visible_depth;
	uint index_table[MAX_STACK_INDEX];
public:
	friend class c_interpreter;
	friend class varity;
	virtual void* find(char*);
	indexed_stack(int esize, void* base_addr, int capacity);
	void* get_layer_begin_pos(void) {return (char*)this->bottom_addr + index_table[current_depth] * element_size;}
	inline void endeep(void) {index_table[++current_depth] = this->count;}
	inline void dedeep(void) {index_table[current_depth--] = 0;}
};

#endif