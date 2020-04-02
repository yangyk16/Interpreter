#pragma once
#ifndef STRUCT_H
#define STRUCT_H
#include "data_struct.h"
#include "config.h"
#include "type.h"

#define MAX_VARITY_COUNT_IN_STRUCT (MAX_FUNCTION_LINE - 2)

class struct_info: public element {
public:
	uint struct_size;//bit31:mem_blk_flag
	stack *varity_stack_ptr;
	PLATFORM_WORD *type_info_ptr;
	int init(char*, stack*);
	uint get_size(void) {return this->struct_size & ~(1 << 31);}
	int get_flag(void) {return (this->struct_size & 1 << 31) != 0;}
	void set_flag(int flag) {this->struct_size = this->struct_size & ~(1 << 31) | (flag << 31);}
	void set_size(uint size) {this->struct_size = this->struct_size & (1 << 31) | size;}
	int dispose(void);
};

class struct_define {
public:
	stack* struct_stack_ptr;
	struct_info* current_node;
	int declare(char*, stack*);
	struct_info* find(char* name) {return (struct_info*)struct_stack_ptr->find(name);}
	struct_info *find_info(void *info_ptr);
	int save_sentence(char* ,uint);
	void init(stack* stack_ptr) {this->struct_stack_ptr = stack_ptr;}
};
#endif
