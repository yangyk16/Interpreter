#include "varity.h"
#include "data_struct.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "error.h"
#include "struct.h"

#if PLATFORM_WORD_LEN == 4
const char type_key[15][19] = {"empty", "struct", "void", "double", "float", "unsigned long long", "long long", "unsigned long", "unsigned int", "long", "int", "unsigned short", "short", "unsigned char", "char"};
const char sizeof_type[] = {0, 0, 0, 8, 4, 8, 8, 4, 4, 4, 4, 2, 2, 1, 1};
const char type_len[] = {5, 6, 4, 6, 5, 18, 9, 13, 12, 4, 3, 14, 5, 13, 4};
#elif PLATFORM_WORD_LEN == 8
const char type_key[15][19] = {"empty", "struct", "void", "double", "float", "unsigned long long", "long long", "unsigned long", "long", "unsigned int", "int", "unsigned short", "short", "unsigned char", "char"};
const char sizeof_type[] = {0, 0, 0, 8, 4, 8, 8, 8, 8, 4, 4, 2, 2, 1, 1};
const char type_len[] = {5, 6, 4, 6, 5, 18, 9, 13, 4, 12, 3, 14, 5, 13, 4};
#endif

bool varity_info::en_echo = 1;
varity_info::varity_info()
{
	memset(this, 0, sizeof(*this));
	//this->content_ptr = 0;
	//this->type = 0;
	//this->size = 0;
	//this->name = 0;
}

void varity_info::config_varity(char attribute, void* info_ptr)
{
	this->attribute |= attribute;
	if(info_ptr)
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
	int name_len = strlen(name);
	this->name = (char*)malloc(name_len + 1);
	strcpy(this->name, name);
	this->type = type;
	this->size = size;
	this->content_ptr = offset;
	this->attribute = 0;
}

void varity_info::init_varity(void* addr, char* name, char type, uint size)
{
	varity_info* varity_ptr = (varity_info*)addr;
	int name_len = strlen(name);
	varity_ptr->name = (char*)vmalloc(name_len+1);
	strcpy(varity_ptr->name, name);
	varity_ptr->type = type;
	varity_ptr->size = size;
	varity_ptr->content_ptr = 0;
	varity_ptr->attribute = 0;
}

varity_info::varity_info(char* name, int type, uint size)
{
	int name_len = strlen(name);
	this->name = (char*)vmalloc(name_len+1);
	strcpy(this->name, name);
	this->type = type;
	this->size = size;
	this->attribute = 0;
	this->content_ptr = 0;
}

int varity_info::get_element_size(void)
{
	if(this->type == STRUCT) {
		return ((struct_info*)this->comlex_info_ptr)->struct_size;
	} else {
		return sizeof_type[this->type];
	}
}

void* varity_info::get_element_ptr(int index)
{
	return (char*)this->get_content_ptr() + index * this->get_element_size();
}

void varity_info::set_to_single(int index)
{
	this->size = this->get_element_size();
	this->content_ptr = this->get_element_ptr(index);
}

void varity_info::convert(void* addr, int type)
{
	if(this->type == VOID) {
		this->type = type;
		this->size = sizeof_type[type];
		this->apply_space();
		memcpy(this->content_ptr, addr, size);
		return;
	}
	if(this->type == type) {
		memcpy(this->content_ptr, addr, this->size);
	} else if(this->type < type) {
		if(this->type == INT || this->type == U_INT || this->type == LONG || this->type == U_LONG)
			*(int*)(this->content_ptr) = *(int*)(addr);
		else if(this->type == U_SHORT || this->type == SHORT)
			*(short*)(this->content_ptr) = *(short*)(addr);
		else if(this->type == U_CHAR)
			*(char*)(this->content_ptr) = *(char*)(addr);
		else if(this->type == DOUBLE) {
			if(type == U_CHAR || type == U_INT || type == U_SHORT || type == U_LONG) {
				*(double*)(this->content_ptr) = *(uint*)(addr);
			} else if(type == CHAR || type == INT || type == SHORT || type == LONG) {
				*(double*)(this->content_ptr) = *(int*)(addr);
			} else if(type == FLOAT) {
				*(double*)(this->content_ptr) = *(float*)(addr);
			}
		} else if(this->type == FLOAT) {
			if(type == U_CHAR || type == U_INT || type == U_SHORT || type == U_LONG) {
				*(float*)(this->content_ptr) = (float)*(uint*)(addr);
			} else if(type == CHAR || type == INT || type == SHORT || type == LONG) {
				*(float*)(this->content_ptr) = (float)*(int*)(addr);
			}
		}
	} else if(this->type > type) {
		if(type == DOUBLE) {
			*(int*)(this->content_ptr) = (int)*(double*)(addr);
		} else if(type == FLOAT) {
			*(int*)(this->content_ptr) = (int)*(float*)(addr);
		} else if(type == INT || type == U_INT){
			*(int*)(this->content_ptr) = *(int*)(addr);
		} else if(this->type == U_SHORT || this->type == SHORT) {
			if(type == DOUBLE) {
				*(short*)(this->content_ptr) = (short)*(double*)(addr);
			} else if(type == FLOAT) {
				*(short*)(this->content_ptr) = (short)*(float*)(addr);
			} else {
				*(short*)(this->content_ptr) = *(short*)(addr);
			}
		} else if(this->type == U_CHAR || this->type == CHAR) {
			if(type == DOUBLE) {
				*(char*)(this->content_ptr) = (char)*(double*)(addr);
			} else if(type == FLOAT) {
				*(char*)(this->content_ptr) = (char)*(float*)(addr);
			} else {
				*(char*)(this->content_ptr) = *(char*)(addr);
			}
		}
	}
	return;
}

void varity_info::create_from_c_varity(void* addr, int type)
{
	if(!this->size) {
		this->type = type;
		this->size = sizeof_type[type];
		this->apply_space();
		memcpy(this->content_ptr, addr, size);
	} else {
		if(this->attribute & ATTRIBUTE_TYPE_UNFIXED) {
			this->type = type;
			this->size = sizeof_type[type];
		}
		this->convert(addr, type);
	}
}

varity_info& varity_info::operator=(const varity_info& source)
{
	this->comlex_info_ptr = source.comlex_info_ptr;
	if(this->attribute & ATTRIBUTE_REFERENCE) {
		this->type = source.type;
		this->size = source.size;
		this->content_ptr = source.content_ptr;
	} else {
		if(!this->size) {
			type = source.type;
			size = source.size;
			attribute = source.attribute;
			comlex_info_ptr = source.comlex_info_ptr;
			this->apply_space();
			memcpy(this->content_ptr, source.content_ptr, size);
		} else {
			if(this->attribute & ATTRIBUTE_TYPE_UNFIXED) {
				this->type = source.type;
				this->size = sizeof_type[this->type];
			}
			this->convert(source.content_ptr, source.type);
		}
	}
	return *this;
}

varity_info& varity_info::operator=(char source) 
{
	create_from_c_varity(&source, CHAR);
	return *this;
}
varity_info& varity_info::operator=(unsigned char source) 
{
	create_from_c_varity(&source, U_CHAR);
	return *this;
}
varity_info& varity_info::operator=(short source) 
{
	create_from_c_varity(&source, SHORT);
	return *this;
}
varity_info& varity_info::operator=(unsigned short source) 
{
	create_from_c_varity(&source, U_SHORT);
	return *this;
}
varity_info& varity_info::operator=(const int& source) 
{
	create_from_c_varity((void*)&source, INT);
	return *this;
}
varity_info& varity_info::operator=(unsigned int source) 
{
	create_from_c_varity(&source, U_INT);
	return *this;
}
varity_info& varity_info::operator=(long long source) 
{
	create_from_c_varity(&source, LONG_LONG);
	return *this;
}
varity_info& varity_info::operator=(unsigned long long source)
{
	create_from_c_varity(&source, U_LONG_LONG);
	return *this;
}
varity_info& varity_info::operator=(float source)
{
	create_from_c_varity(&source, FLOAT);
	return *this;
}
varity_info& varity_info::operator=(double source)
{
	create_from_c_varity(&source, DOUBLE);
	return *this;
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

int varity::declare(int scope_flag, char* name, char type, uint size, char attribute)
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
	else
		varity_stack = analysis_varity_stack;
	if(varity_stack->is_full()) {
		error("declare varity \"%s\" error: varity count reach max\n", name);
		return ERROR_VARITY_COUNT_MAX;
	}
	varity_ptr = (varity_info*)varity_stack->get_current_ptr();
	varity_info::init_varity(varity_ptr, name, type, size);
	varity_ptr->config_varity(attribute);
	if(attribute != ATTRIBUTE_LINK) {
		ret = varity_ptr->apply_space();
		if(ret) {
			error("declare varity \"%s\" error: space is insufficient\n", name);
			return ERROR_VARITY_NOSPACE;
		}
	}
	varity_stack->push();
	return 0;
}

void varity_attribute::init(void* addr, char* name, char type, char attribute, uint size)
{
	int name_len = strlen(name);
	varity_attribute* ptr = (varity_attribute*)addr;
	ptr->type = type;
	ptr->attribute = attribute;
	ptr->size = size;
	ptr->name = (char*)vmalloc(name_len + 1);
	strcpy(ptr->name, name);
}

varity_info* varity::find(char* name, int scope)
{
	varity_info* ret = NULL;
	if(scope & PRODUCED_ANALIES) {
		ret = (varity_info*)this->analysis_varity_stack->find(name);
		if(ret)
			return ret;
	}
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

int varity::declare_analysis_varity(char type, uint size, char* ret_name, varity_info** varity_ptr, char attribute)
{
	int ret;
	char tmp_varity_name[3];
	sprintf(tmp_varity_name, "%c%c", TMP_VAIRTY_PREFIX, 128 + this->cur_analysis_varity_count++);
	if(ret_name)
		strcpy(ret_name, tmp_varity_name);
	ret = this->declare(VARITY_SCOPE_ANALYSIS, tmp_varity_name, type, size, attribute);
	//if(ret_name)
	//	*varity_ptr = (varity_info*)this->analysis_varity_stack->find(ret_name);
	*varity_ptr = (varity_info*)this->analysis_varity_stack->get_lastest_element();
	return ret;
}

int varity::destroy_analysis_varity(int count)
{
	varity_info* varity_ptr;
	for(int i=0; i<count; i++) {
		varity_ptr = (varity_info*)this->analysis_varity_stack->pop();
		varity_ptr->reset();
	}
	this->cur_analysis_varity_count -= count;
	return 0;
}

int varity::destroy_analysis_varity(void)
{
	int count = this->analysis_varity_stack->get_count();
	varity_info* varity_ptr;
	for(int i=0; i<count; i++) {
		varity_ptr = (varity_info*)this->analysis_varity_stack->pop();
		varity_ptr->reset();
	}
	this->cur_analysis_varity_count = 0;
	return 0;
}

int varity::destroy_local_varity_cur_depth(void)
{
	varity_info* varity_ptr;
	//varity_info* layer_begin_pos = (varity_info*)this->local_varity_stack->get_layer_begin_pos();
	//while(this->local_varity_stack->get_count() > 0 && (int)(varity_ptr = (varity_info*)this->local_varity_stack->get_lastest_element()) - (int)layer_begin_pos >= 0) {
	while(this->local_varity_stack->get_count() > this->local_varity_stack->index_table[this->local_varity_stack->current_depth]) {
		varity_ptr = (varity_info*)this->local_varity_stack->get_lastest_element();
		this->local_varity_stack->pop();
		if(varity_ptr)
			varity_ptr->reset();
		else {
			error("data struct error,varity.cpp: %d\n", __LINE__);
			return 1;
		}
	}
	return 0;
}

int varity::undeclare(void)
{
	return 0;
}

varity::varity(stack* g_stack, indexed_stack* l_stack, stack* a_stack)
{
	this->global_varity_stack = g_stack;
	this->local_varity_stack = l_stack;
	this->analysis_varity_stack = a_stack;
	this->cur_analysis_varity_count = 0;
	this->current_stack_depth = 0;
}