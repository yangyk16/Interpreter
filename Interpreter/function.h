#pragma once
#ifndef FUNCTION_H
#define FUNCTION_H
#include "data_struct.h"

class function_info: public element {
	char* buffer;
	int wptr;
	int rptr;
public:
	char variable_para_flag;
	char compile_func_flag;
	void* func_addr;
	stack mid_code_stack;
	int row_line;
	char** row_begin_pos;
	int* row_len;
	int stack_frame_size;
	stack* arg_list;
	int init(char*, stack*);
	int init(char *name, void* addr, stack *arg_list, char variable_arg_flag);
	int reset(void);
	int save_sentence(char* ,uint);
	int size_adapt(void);
};

class function {
	function_info* current_node;
	stack* function_stack_ptr;
public:
	void init(stack*);
	function_info* find(char* name) {return (function_info*)this->function_stack_ptr->find(name);}
	function_info* get_current_node(void) {return this->current_node;}
	void current_node_abort(void);
	int save_sentence(char* ,uint);
	int declare(char*, stack*);
	int add_compile_func(char *name, void *addr, stack *arg_list, char variable_arg_flag);
};
#endif
