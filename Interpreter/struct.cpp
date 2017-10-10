#include "struct.h"
#include "config.h"
#include "error.h"
#include <stdio.h>

int struct_define::save_sentence(char* str, uint len)
{
	return this->current_node->save_sentence(str, len);
}

int struct_define::declare(char* name, stack* arg_list)
{
	struct_info* struct_node_ptr;

	if(this->struct_stack_ptr->find(name)) {
		error("declare struct \"%s\" error: struct name duplicated\n", name);
		return ERROR_FUNC_DUPLICATE;
	}

	if(struct_stack_ptr->is_full()) {
		error("declare struct \"%s\" error: struct count reach max\n", name);
		return ERROR_FUNC_COUNT_MAX;
	}
	struct_node_ptr = (function_info*)struct_stack_ptr->get_current_ptr();
	struct_node_ptr->init(name, arg_list);
	this->current_node = struct_node_ptr;
	struct_stack_ptr->push();
	return 0;
}