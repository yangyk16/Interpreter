#include "struct.h"
#include "config.h"
#include "error.h"
#include <stdio.h>
#include <string.h>
#include "varity.h"
#include <stdlib.h>

int struct_info::init(char* name, stack* varity_list)
{//TODO: mallocÊ§°Ü
	int name_len = strlen(name);
	this->name = (char*)vmalloc(name_len + 1);
	strcpy(this->name, name);
	this->varity_stack_ptr = varity_list;
	return 0;
}

int struct_info::reset(void)
{
	if(this->name)
		vfree(this->name);
	if(this->varity_stack_ptr) {
		vfree(this->varity_stack_ptr->visit_element_by_index(0));
		vfree(this->varity_stack_ptr);
	}
	return 0;
}

varity_info& struct_info::visit_struct_member(void* struct_content_ptr, varity_info* member_varity_ptr)
{
	static varity_info ret;
	memcpy(&ret, member_varity_ptr, sizeof(varity_info));
	if(ret.get_type() == STRUCT) {
		//TODO: struct as member in other struct.
	}
	ret.config_varity(ATTRIBUTE_LINK);
	ret.set_content_ptr((void*)((uint)ret.get_content_ptr() + (uint)struct_content_ptr));
	return ret;
}

int struct_define::save_sentence(char* str, uint len)
{
	return 0;
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
	struct_node_ptr = (struct_info*)struct_stack_ptr->get_current_ptr();
	struct_node_ptr->init(name, arg_list);
	this->current_node = struct_node_ptr;
	struct_stack_ptr->push();
	return 0;
}