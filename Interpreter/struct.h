#pragma once
#ifndef STRUCT_H
#define STRUCT_H
#include "data_struct.h"
#include "function.h"
#include "config.h"

#define MAX_VARITY_COUNT_IN_STRUCT (MAX_FUNCTION_LINE - 2)

typedef class function_info struct_info;//TODO: ¸ÄÎª¼Ì³Ğ£¬ÖØĞ´reset
/*class struct_info {
	char* name;
	stack* varity_stack;
};*/

class struct_define {
	stack* struct_stack_ptr;
public:
	struct_info* current_node;
	int declare(char*, stack*);
	struct_info* find(char*);
	int save_sentence(char* ,uint);
	struct_define(stack* struct_stack_ptr) {this->struct_stack_ptr = struct_stack_ptr;}
};
#endif