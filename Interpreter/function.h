#pragma once
#ifndef FUNCTION_H
#define FUNCTION_H
#include "data_struct.h"

class mid_code;
class function_info: public element {
	char *buffer;
	int wptr;
	int rptr;
public:
	char variable_para_flag;
	char compile_func_flag;
	void *func_addr;
	stack mid_code_stack;
	int row_line;
	char **row_begin_pos;
#if DEBUG_EN
	mid_code **row_code_ptr;
	indexed_stack local_varity_stack;
#endif
	int *row_len;
	int stack_frame_size;
	stack* arg_list;
	int init(char*, stack*);
	int init(char *name, void* addr, stack *arg_list, char variable_arg_flag);
	int reset(void);
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
	stack* function_stack_ptr;
public:
	void init(stack*);
	function_info* find(char* name) {return (function_info*)this->function_stack_ptr->find(name);}
	function_info* get_current_node(void) {return this->current_node;}
	void current_node_abort(void);
	int save_sentence(char* ,uint);
	int destroy_sentence(void);
	int declare(char*, stack*);
	int add_compile_func(char *name, void *addr, stack *arg_list, char variable_arg_flag);
};
#endif
