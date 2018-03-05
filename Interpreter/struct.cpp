#include "struct.h"
#include "config.h"
#include "error.h"
#include "varity.h"
#include <stdlib.h>
#include "cstdlib.h"

int struct_info::init(char* name, stack* varity_list)
{//TODO: malloc失败
	int name_len = kstrlen(name);
	this->name = (char*)vmalloc(name_len + 1);
	kstrcpy(this->name, name);
	this->varity_stack_ptr = varity_list;
	kmemcpy(this->type_info_ptr, basic_type_info[STRUCT], sizeof(int) * 3);
	this->type_info_ptr[1] = (uint)this;
	this->type_info_ptr[0] = 1;//初始引用次数为1
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
	kmemcpy(&ret, member_varity_ptr, sizeof(varity_info));
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

struct_info *struct_define::find_info(void *info_ptr)
{
	struct_info *cur_struct_ptr = (struct_info*)struct_stack_ptr->get_base_addr();
	int count = struct_stack_ptr->get_count();
	for(int i=0; i<count; i++, cur_struct_ptr++) {
		if(cur_struct_ptr == info_ptr)
			return cur_struct_ptr;
	}
	return NULL;
}

int struct_define::declare(char* name, stack* arg_list)
{
	struct_info* struct_node_ptr;

	if(struct_stack_ptr->find(name)) {
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
