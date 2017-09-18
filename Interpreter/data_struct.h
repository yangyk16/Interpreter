#pragma once
#ifndef DATA_STRUCTURE
#define DATA_STRUCTURE

#include "type.h"
#include "config.h"

class element {
protected:
	char*	name;
};

class data_struct {
protected:
	uint element_size;
};

class round_queue: public data_struct {
	uint wptr;
	uint rptr;
	uint length;
	void* bottom_addr;
	uint count;
public:
	round_queue();
	round_queue(uint, void*);
	inline void set_base(void* addr) {this->bottom_addr = addr;}
	inline void set_length(uint len) {this->length = len;}
	int write(void*, uint);
	int read(void*, uint);
	int readline(char*);
};

class stack: public data_struct {
protected:
	uint top;
	void* bottom_addr;
public:
	void push(void*);
	void* pop(void);
	void* find(void*);
	inline void set_base(void* addr) {this->bottom_addr = addr;}
	//inline void set_length(uint len) {this->length = len;}
	stack();
	stack(int esize, void* base_addr);
};

class indexed_stack: public stack {
	int current_depth;
	uint index_table[MAX_STACK_INDEX];
public:
	indexed_stack(int esize, void* base_addr);
	inline void endeep(void) {index_table[current_depth] = top; current_depth++;}
	inline void dedeep(void) {current_depth--;}
};

#endif