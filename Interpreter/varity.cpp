#include "varity.h"
#include "data_struct.h"
#include <stdio.h>
#include "error.h"
#include "struct.h"
#include "operator.h"
#include "string_lib.h"
#include "cstdlib.h"

#if PLATFORM_WORD_LEN == 4
const char type_key[15][19] = {"empty", "char", "unsigned char", "short", "unsigned short", "int", "long", "unsigned int", "unsigned long", "long long", "unsigned long long", "float", "double", "void", "struct"};
const char sizeof_type[] = {0, 1, 1, 2, 2, 4, 4, 4, 4, 8, 8, 4, 8, 0, 0};
const char type_len[] = {5, 4, 13, 5, 14, 3, 4, 12, 13, 9, 18, 5, 6, 4, 6};
PLATFORM_WORD basic_type_info[15][4] = {{1, BASIC_TYPE_SET(COMPLEX)}, {1, BASIC_TYPE_SET(CHAR), SET_COMPLEX_TYPE(COMPLEX_PTR)}, {1, BASIC_TYPE_SET(U_CHAR), SET_COMPLEX_TYPE(COMPLEX_PTR)}, {1, BASIC_TYPE_SET(SHORT), SET_COMPLEX_TYPE(COMPLEX_PTR)}, {1, BASIC_TYPE_SET(U_SHORT), SET_COMPLEX_TYPE(COMPLEX_PTR)}, {1, BASIC_TYPE_SET(INT), SET_COMPLEX_TYPE(COMPLEX_PTR)}, {1, BASIC_TYPE_SET(LONG), SET_COMPLEX_TYPE(COMPLEX_PTR)}, {1, BASIC_TYPE_SET(U_INT), SET_COMPLEX_TYPE(COMPLEX_PTR)}, {1, BASIC_TYPE_SET(U_LONG), SET_COMPLEX_TYPE(COMPLEX_PTR)}, {1, BASIC_TYPE_SET(LONG_LONG), SET_COMPLEX_TYPE(COMPLEX_PTR)}, {1, BASIC_TYPE_SET(U_LONG_LONG), SET_COMPLEX_TYPE(COMPLEX_PTR)}, {1, BASIC_TYPE_SET(FLOAT), SET_COMPLEX_TYPE(COMPLEX_PTR)}, {1, BASIC_TYPE_SET(DOUBLE), SET_COMPLEX_TYPE(COMPLEX_PTR)}, {1, BASIC_TYPE_SET(VOID), SET_COMPLEX_TYPE(COMPLEX_PTR)}, {1, 0, BASIC_TYPE_SET(STRUCT), SET_COMPLEX_TYPE(COMPLEX_PTR)}};
#elif PLATFORM_WORD_LEN == 8
const char type_key[15][19] = {"empty", "char", "unsigned char", "short", "unsigned short", "int", "unsigned int", "long", "long long", "unsigned long", "unsigned long long", "float", "double", "void", "struct"};
const char sizeof_type[] = {0, 0, 0, 8, 4, 8, 8, 8, 8, 4, 4, 2, 2, 1, 1};
const char type_len[] = {5, 6, 4, 6, 5, 18, 9, 13, 4, 12, 3, 14, 5, 13, 4};
#endif

bool varity_info::en_echo = 1;

void inc_varity_ref(varity_info *varity_ptr)
{
	PTR_N_VALUE(varity_ptr->get_complex_ptr())++;
	debug("inc %x\n", varity_ptr);
}

void dec_varity_ref(varity_info *varity_ptr, bool destroy_flag)
{
	int remain_count = --PTR_N_VALUE(varity_ptr->get_complex_ptr());
	if(destroy_flag && !remain_count)
		vfree(varity_ptr->get_complex_ptr());
	debug("dec %x\n", varity_ptr);
}

int destroy_varity_stack(stack *stack_ptr)//仅限局部变量和参数表
{
	varity_info *varity_ptr = (varity_info*)stack_ptr->get_base_addr();
	for(int i=0; i<stack_ptr->get_count(); i++, varity_ptr++) {
		dec_varity_ref(varity_ptr, true);
		vfree(varity_ptr->get_name());
	}
	return ERROR_NO;
}

varity_info::varity_info()
{
	kmemset(this, 0, sizeof(*this));
}

void varity_info::config_varity(char attribute, void* info_ptr)
{
	this->attribute |= attribute;
	if(info_ptr)
		this->comlex_info_ptr = (PLATFORM_WORD*)info_ptr;
}

int varity_attribute::get_type(void)
{
	if(this->complex_arg_count == 1)
		return GET_COMPLEX_DATA(this->comlex_info_ptr[1]);
	else if(this->complex_arg_count == 2 && GET_COMPLEX_DATA(this->comlex_info_ptr[2]) == STRUCT && GET_COMPLEX_TYPE(this->comlex_info_ptr[2]) == COMPLEX_BASIC)
		return STRUCT;
	else {
		if(GET_COMPLEX_TYPE(this->comlex_info_ptr[this->complex_arg_count]) == COMPLEX_PTR) {
			return PTR;
		} else if(GET_COMPLEX_TYPE(this->comlex_info_ptr[this->complex_arg_count]) == COMPLEX_ARRAY) {
			return ARRAY;
		}
	}
}

void varity_info::config_complex_info(int complex_arg_count, PLATFORM_WORD* info_ptr)
{
	this->complex_arg_count = complex_arg_count;
	this->comlex_info_ptr = info_ptr;
}

void varity_info::clear_attribute(char attribute)
{
	this->attribute &= ~attribute;
}

int varity_info::struct_apply(void)
{
	this->size = ((struct_info*)this->comlex_info_ptr)->struct_size;
	this->apply_space();
	return 0;
}

void varity_info::arg_init(char* name, char type, uint size, void* offset)
{
	int name_len = kstrlen(name);
	this->name = (char*)vmalloc(name_len + 1);
	kstrcpy(this->name, name);
	this->type = type;
	this->size = size;
	this->content_ptr = offset;
	this->attribute = 0;
	if(type != COMPLEX) {
		this->comlex_info_ptr = basic_type_info[type];
		inc_varity_ref(this);
		if(type != STRUCT) {
			this->complex_arg_count = 1;
		} else {
			this->complex_arg_count = 2;
		}
	}
}

void varity_info::init_varity(void* addr, char* name, char type, uint size)
{
	varity_info* varity_ptr = (varity_info*)addr;
	if(name) {
		int name_len = kstrlen(name);
		varity_ptr->name = (char*)vmalloc(name_len+1);
		kstrcpy(varity_ptr->name, name);
	}
	varity_ptr->type = type;
	varity_ptr->size = size;
	varity_ptr->content_ptr = 0;
	varity_ptr->attribute = 0;
	if(type != COMPLEX) {
		varity_ptr->comlex_info_ptr = basic_type_info[type];
		inc_varity_ref(varity_ptr);
		if(type != STRUCT) {
			varity_ptr->complex_arg_count = 1;
		} else {
			varity_ptr->complex_arg_count = 2;
		}
	}
}

varity_info::varity_info(char* name, int type, uint size)
{
	int name_len = kstrlen(name);
	this->name = (char*)vmalloc(name_len+1);
	kstrcpy(this->name, name);
	this->type = type;
	this->size = size;
	this->attribute = 0;
	this->content_ptr = 0;
}

void varity_info::set_type(int type)
{
	if(type < PTR) {
		this->comlex_info_ptr = basic_type_info[type];
		this->complex_arg_count = 1;
	}
}

int varity_info::get_first_order_sub_struct_size(void)
{
	if(GET_COMPLEX_TYPE(this->comlex_info_ptr[this->complex_arg_count]) == COMPLEX_PTR || GET_COMPLEX_TYPE(this->comlex_info_ptr[this->complex_arg_count]) == COMPLEX_ARRAY)
		return get_varity_size(0, this->comlex_info_ptr, this->complex_arg_count - 1);
	else
		return ERROR_NO_SUB_STRUCT;
}

int varity_info::get_element_size(void)
{
	return ::get_element_size(this->complex_arg_count, this->comlex_info_ptr);//成员函数访问全局同名普通函数加上::
}

int varity_info::apply_space(void)
{
	if(this->content_ptr && !(this->attribute & ATTRIBUTE_LINK)) {
		vfree(this->content_ptr);
		this->content_ptr = 0;
	}
	if(this->size) {
		this->content_ptr = vmalloc(this->size);
		if(this->content_ptr)
			return 0;
		else
			return 1;
	}
	return 0;
}

void varity_info::reset(void)
{
	this->size = 0;
	this->type = 0;
	if(this->content_ptr && !(this->attribute & ATTRIBUTE_LINK)) {
		vfree(this->content_ptr);
	}
	this->content_ptr = 0;
	if(this->name) {
		vfree(this->name);
		this->name = 0;
	}
	this->attribute = 0;
}

int varity_info::is_non_zero(void)
{
	if(type >= U_LONG_LONG) {
		if(*(long long*)this->content_ptr)
			return 1;
	} else if(type == FLOAT) {
		if(*(float*)this->content_ptr != 0)
			return 1;
	} else if(type == DOUBLE) {
		if(*(double*)this->content_ptr != 0)
			return 1;
	}
	return 0;
}

void varity_info::echo(void)
{
	if(en_echo) {
		if(this->type == INT || this->type == LONG || this->type == SHORT || this->type == CHAR || this->type == U_SHORT || this->type == U_CHAR) {
			debug("%s = %d\n",this->name, *(int*)this->content_ptr);
		} else if(this->type == U_INT || this->type == U_LONG || this->type == U_SHORT || this->type == U_CHAR)
			debug("%s = %lu\n",this->name, *(unsigned int*)this->content_ptr);
		else if(this->type == DOUBLE)
			debug("%s = %f\n",this->name, *(double*)this->content_ptr);
		else if(this->type == FLOAT)
			debug("%s = %f\n",this->name, *(float*)this->content_ptr);
		else if(this->type >= BASIC_VARITY_TYPE_COUNT) {
			debug("%s = 0x%x\n",this->name, *(int*)this->content_ptr);
		}
	}
}

int varity::declare(int scope_flag, char *name, char type, uint size, int complex_arg_count, PLATFORM_WORD *complex_info_ptr)
{//scope_flag = 0:global; 1: local; 2:analysis_tmp
	int ret;
	varity_info* varity_ptr;
	stack* varity_stack;
	if(scope_flag == VARITY_SCOPE_GLOBAL) {
		if(this->global_varity_stack->find(name)) {
			error("declare varity \"%s\" error: varity name duplicated\n", name);
			return ERROR_VARITY_DUPLICATE;
		}
	} else if(scope_flag == VARITY_SCOPE_LOCAL) {
		if(this->local_varity_stack->find(name)) {
			error("declare varity \"%s\" error: varity name duplicated\n", name);
			return ERROR_VARITY_DUPLICATE;
		}
	}
	if(scope_flag == VARITY_SCOPE_GLOBAL)
		varity_stack = global_varity_stack;
	else if(scope_flag == VARITY_SCOPE_LOCAL)
		varity_stack = local_varity_stack;
	if(varity_stack->is_full()) {
		error("declare varity \"%s\" error: varity count reach max\n", name);
		return ERROR_VARITY_COUNT_MAX;
	}
	varity_ptr = (varity_info*)varity_stack->get_current_ptr();
	varity_info::init_varity(varity_ptr, name, type, size);
	if(complex_arg_count && complex_info_ptr)
		varity_ptr->config_complex_info(complex_arg_count, complex_info_ptr);
	if(scope_flag == VARITY_SCOPE_GLOBAL) {
		ret = varity_ptr->apply_space();
		if(ret) {
			error("declare varity \"%s\" error: space is insufficient\n", name);
			return ERROR_VARITY_NOSPACE;
		}
	} else if(scope_flag == VARITY_SCOPE_LOCAL) {
		varity_ptr->set_content_ptr((void*)make_align(this->local_varity_stack->offset, varity_ptr->get_element_size()));
		this->local_varity_stack->offset += varity_ptr->get_size();
	}
	varity_stack->push();
	return 0;
}

void varity_attribute::init(void* addr, char* name, char type, char attribute, uint size)
{
	int name_len = kstrlen(name);
	varity_attribute* ptr = (varity_attribute*)addr;
	ptr->type = type;
	ptr->attribute = attribute;
	ptr->size = size;
	if(name) {
		ptr->name = (char*)vmalloc(name_len + 1);
		kstrcpy(ptr->name, name);
	}
}

varity_info* varity::vfind(char *name, int &scope)
{
	varity_info* ret = NULL;
	ret = (varity_info*)this->local_varity_stack->find(name);
	if(ret) {
		scope = VARITY_SCOPE_LOCAL;
	} else {
		ret = (varity_info*)this->global_varity_stack->find(name);
		scope = VARITY_SCOPE_GLOBAL;
	}
	return ret;
}

varity_info* varity::find(char* name, int scope)
{
	varity_info* ret = NULL;
	if(scope & PRODUCED_DECLARE) {
		ret = (varity_info*)this->local_varity_stack->find(name);
		if(ret)
			return ret;
		else
			ret = (varity_info*)this->global_varity_stack->find(name);
		if(ret)
			return ret;
	}
	return ret;
}

int varity::destroy_local_varity_cur_depth(void)
{
	varity_info* varity_ptr;
	//varity_info* layer_begin_pos = (varity_info*)this->local_varity_stack->get_layer_begin_pos();
	//while(this->local_varity_stack->get_count() > 0 && (int)(varity_ptr = (varity_info*)this->local_varity_stack->get_lastest_element()) - (int)layer_begin_pos >= 0) {
	while(this->local_varity_stack->get_count() > this->local_varity_stack->index_table[this->local_varity_stack->current_depth]) {
		varity_ptr = (varity_info*)this->local_varity_stack->get_lastest_element();
		this->local_varity_stack->pop();
		if(varity_ptr) {
			dec_varity_ref(varity_ptr, true);
			vfree(varity_ptr->get_name());
		} else {
			error("data struct error,varity.cpp: %d\n", __LINE__);
			return 1;
		}
	}
	return 0;
}

int varity::destroy_local_varity(void)
{
	varity_info *local_varity_ptr = (varity_info*)this->local_varity_stack->get_base_addr();
	for(int i=0; i<this->local_varity_stack->count; i++, local_varity_ptr++) {
		dec_varity_ref(local_varity_ptr, true);
		vfree(local_varity_ptr->get_name());
	}
	this->local_varity_stack->reset();
	return 0;
}

int varity::undeclare(void)
{
	return 0;
}

void varity::init(stack* g_stack, indexed_stack* l_stack)
{
	this->global_varity_stack = g_stack;
	this->local_varity_stack = l_stack;
	this->cur_analysis_varity_count = 0;
	this->current_stack_depth = 0;
}

int get_varity_size(int basic_type, PLATFORM_WORD *complex_info, int complex_arg_count)
{
	int varity_size = 0;
	int &n = complex_arg_count;
	if(basic_type <= VOID && basic_type > CHAR)
		return sizeof_type[basic_type];
	else if(basic_type == STRUCT) {
		return ((struct_info*)complex_info[1])->struct_size;
	}
	for(int i=1; i<n+1; i++) {
		switch(GET_COMPLEX_TYPE(complex_info[i])) {
		case COMPLEX_BASIC:
			if(GET_COMPLEX_DATA(complex_info[i]) != STRUCT)
				varity_size = sizeof_type[complex_info[i] & COMPLEX_DATA_BIT_MASK];
			else
				varity_size = ((struct_info*)complex_info[i - 1])->struct_size;//TODO:为严谨性应先避免struct信息地址正好命中case中的情况，改用自顶向下的递归可实现
			break;
		case COMPLEX_PTR:
			varity_size = PLATFORM_WORD_LEN;
			break;
		case COMPLEX_ARRAY:
			varity_size *= complex_info[i] & COMPLEX_DATA_BIT_MASK;
			break;
		}
	}
	return varity_size;
}

int array_to_ptr(PLATFORM_WORD *&complex_info, int complex_arg_count)
{
	if(GET_COMPLEX_TYPE(complex_info[complex_arg_count]) != COMPLEX_ARRAY)
		return -1;//TODO:找一个返回值
	PLATFORM_WORD *new_complex_info = (PLATFORM_WORD*)vmalloc((complex_arg_count + 1) * sizeof(PLATFORM_WORD));
	kmemcpy(new_complex_info + 1, complex_info + 1, (complex_arg_count - 1) * sizeof(PLATFORM_WORD));
	new_complex_info[complex_arg_count] = COMPLEX_PTR << COMPLEX_TYPE_BIT;
	complex_info = new_complex_info;
	return 0;
}

void *get_basic_info(int basic_type, void *info_ptr, struct_define *struct_define_ptr)
{
	if(basic_type >= CHAR && basic_type <= VOID)
		return basic_type_info[basic_type];
	else if(basic_type == STRUCT) {
		//struct类中加入info*，匹配info_ptr，找到struct的info
		struct_info *struct_info_ptr = struct_define_ptr->find_info(info_ptr);
		if(struct_info_ptr)
			return struct_info_ptr->type_info_ptr;
	} else {
		return 0;
	}
}

int get_element_size(int complex_arg_count, PLATFORM_WORD *comlex_info_ptr)
{
	int i;
	for(i=complex_arg_count; i>0; i--) {
		if(GET_COMPLEX_TYPE(comlex_info_ptr[i]) != COMPLEX_ARRAY) {
			break;
		}
	}
	return get_varity_size(0, comlex_info_ptr, i);
}