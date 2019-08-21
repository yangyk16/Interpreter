#pragma once
#ifndef MACRO_H
#define MACRO_H

#include "config.h"
#include "data_struct.h"

class macro_info: public element {
public:
	char *macro_arg_name[MAX_MACRO_ARG_COUNT];
	char *macro_instead_str;
	void init(char*, char**, char*);
	int find(char*);
};

class macro {
public:
	stack* macro_stack_ptr;
	void init(stack*);
	macro_info* find(const char* name) {return (macro_info*)this->macro_stack_ptr->find(name);}
	int declare(char*, char**, char*);
};
#endif