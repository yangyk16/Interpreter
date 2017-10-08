#pragma once
#ifndef FUNCTION_H
#define FUNCTION_H
#include "data_struct.h"

class function_info: public element {
	char* buffer;
	int wptr;
	int rptr;
public:
	int row_line;
	char** row_begin_pos;
	int* row_len;
	char* analysis_buf;
	stack* arg_list;
	int init(char*, stack*);
	int save_sentence(char* ,uint);
};

class function {
	function_info* current_node;
	stack* function_stack_ptr;
public:
	function(stack*);
	void* find(char* name) {return this->function_stack_ptr->find(name);}
	void current_node_abort(void);
	int save_sentence(char* ,uint);
	int declare(char*, stack*);

};
#endif
