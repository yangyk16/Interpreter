#pragma once
#ifndef DATA_STRUCTURE
#define DATA_STRUCTURE

#include "type.h"
#include "config.h"

class element {
	char*	name;
};

class data_struct {
protected:
	uint element_size;
public:
	virtual void push(void*) = 0;
	virtual void* pop(void) = 0;
	virtual void* find(void*) = 0;
};

class stack: public data_struct {
protected:
	uint top;
	void* bottom_addr;
public:
	virtual void push(void*);
	virtual void* pop(void);
	virtual void* find(void*);
	stack();
	stack(int esize, void* base_addr);
};

class indexed_stack: public stack {
	int current_depth;
	uint index_table[MAX_FUNC_CALL_DEPTH];
public:
	inline void endeep(void) {current_depth++; index_table[current_depth] = top;}
};

#endif