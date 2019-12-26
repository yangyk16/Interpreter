#pragma once
#ifndef FUNCTION_H
#define FUNCTION_H
#include "data_struct.h"

#define FUNC_FLAG_COMPILE	1
#define FUNC_FLAG_PROTOTYPE	2

class mid_code;
class function_info: public element {
public:
	char *buffer;
	int wptr;
	char variable_para_flag;
	char compile_func_flag;
	char debug_flag;
	void *func_addr;
	stack mid_code_stack;
	int row_line;
	unsigned int *row_begin_pos;
	unsigned int data_size;
	int stack_frame_size;
	stack* arg_list;
#if DEBUG_EN
	mid_code **row_code_ptr;
	indexed_stack local_varity_stack;
#endif
	int init(char*, stack*, int);
	int init(const char *name, void* addr, stack *arg_list, char variable_arg_flag);
	int dispose(void);
	int save_sentence(char* ,uint);
#if DEBUG_EN
	int copy_local_varity_stack(indexed_stack *lvsp);
#endif
	int print_line(int line);
	int destroy_sentence(void);
	int size_adapt(void);
};

class function {
	function_info* current_node;
public:
	stack* function_stack_ptr;
	void init(stack*);
	function_info* find(const char* name) {return (function_info*)this->function_stack_ptr->find(name);}
	function_info* get_current_node(void) {return this->current_node;}
	void current_node_abort(void);
	int save_sentence(char* ,uint);
	int destroy_sentence(void);
	int declare(char*, stack*, int);
	int add_compile_func(const char *name, void *addr, stack *arg_list, char variable_arg_flag);
};
#endif
