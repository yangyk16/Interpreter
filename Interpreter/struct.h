#pragma once
#ifndef STRUCT_H
#define STRUCT_H
#include "data_struct.h"
#include "function.h"
#include "config.h"
#include "type.h"
#include "varity.h"

#define MAX_VARITY_COUNT_IN_STRUCT (MAX_FUNCTION_LINE - 2)

class struct_info: public element {
public:
	uint struct_size;
	stack *varity_stack_ptr;
	uint type_info_ptr[4];
	int init(char*, stack*);
	int reset(void);
	varity_info& visit_struct_member(void* struct_content_ptr, varity_info* member_varity_ptr);
};

class struct_define {
	stack* struct_stack_ptr;
public:
	struct_info* current_node;
	int declare(char*, stack*);
	struct_info* find(char* name) {return (struct_info*)struct_stack_ptr->find(name);}
	struct_info *find_info(void *info_ptr);
	int save_sentence(char* ,uint);
	struct_define(stack* stack_ptr) {struct_define::struct_stack_ptr = stack_ptr;}
};
#endif