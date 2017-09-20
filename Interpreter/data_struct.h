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
};

class round_queue: public data_struct {
	uint wptr;
	uint rptr;
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
public:
	void push(void*);
	void* pop(void);
	void* find(char*);
	inline void set_base(void* addr) {this->bottom_addr = addr;}
	//inline void set_length(uint len) {this->length = len;}
	stack();
	stack(int esize, void* base_addr, int capacity);
};

class indexed_stack: public stack {
	int current_depth;
	uint index_table[MAX_STACK_INDEX];
public:
	indexed_stack(int esize, void* base_addr, int capacity);
	inline void endeep(void) {index_table[current_depth] = top; current_depth++;}
	inline void dedeep(void) {current_depth--;}
};

#endif