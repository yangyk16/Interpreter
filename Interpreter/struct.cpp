#include "struct.h"
#include "config.h"
#include "error.h"
#include "varity.h"
#include <stdlib.h>
#include "cstdlib.h"
#include "interpreter.h"
#include "global.h"

int struct_info::init(char* name, stack* varity_list)
{//TODO: malloc失败
	int name_len = kstrlen(name);
	PLATFORM_WORD *type = (PLATFORM_WORD*)dmalloc(4 * PLATFORM_WORD_LEN, "struct type");
	string_info *string_ptr;
	string_ptr = (string_info*)name_stack.find(name);
	this->name = string_ptr->get_name();
	//this->name = (char*)dmalloc(name_len + 1, "");
	//kstrcpy(this->name, name);
	this->varity_stack_ptr = varity_list;
	kmemcpy(type, basic_type_info[STRUCT], PLATFORM_WORD_LEN * 4);
	type[1] = (PLATFORM_WORD)this;
	varity_type_stack.push(3, type);
	this->type_info_ptr = type;//初始引用次数为1
	return 0;
}

int struct_info::dispose(void)
{
	//destroy_varity_stack(this->varity_stack_ptr);
	if(this->varity_stack_ptr) {
		vfree(this->varity_stack_ptr->visit_element_by_index(0));//TODO:检查是否释放所有成员
		vfree(this->varity_stack_ptr);
	}
	return 0;
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
