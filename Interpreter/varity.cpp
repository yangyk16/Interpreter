#include "varity.h"
#include "data_struct.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if PLATFORM_WORD_LEN == 4
const char type_key[13][19] = {"void", "double", "float", "unsigned long long", "long long", "unsigned long", "unsigned int", "long", "int", "unsigned short", "short", "unsigned char", "char"};
const char sizeof_type[] = {0, 8, 4, 8, 8, 4, 4, 4, 4, 2, 2, 1, 1};
#elif PLATFORM_WORD_LEN == 8
const char type_key[13][19] = {"void", "double", "float", "unsigned long long", "long long", "unsigned long", "long", "unsigned int", "int", "unsigned short", "short", "unsigned char", "char"};
const char sizeof_type[] = {0, 8, 4, 8, 8, 8, 8, 4, 4, 2, 2, 1, 1};
#endif

bool varity_info::en_echo = 1;
varity_info::varity_info()
{
	this->content_ptr = 0;
	this->type = 0;
	this->size = 0;
	this->name = 0;
}

void varity_info::init_varity(void* addr, char* name, int type, uint size)
{
	varity_info* varity_ptr = (varity_info*)addr;
	int name_len = strlen(name);
	varity_ptr->name = (char*)malloc(name_len+1);
	strcpy(varity_ptr->name, name);
	varity_ptr->type = type;
	varity_ptr->size = size;
	varity_ptr->content_ptr = 0;
}

varity_info::varity_info(char* name, int type, uint size)
{
	int name_len = strlen(name);
	this->name = (char*)malloc(name_len+1);
	strcpy(this->name, name);
	this->type = type;
	this->size = size;
	this->content_ptr = 0;
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
		memcpy(this->content_ptr, addr, sizeof_type[type]);
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
				*(float*)(this->content_ptr) = *(uint*)(addr);
			} else if(type == CHAR || type == INT || type == SHORT || type == LONG) {
				*(float*)(this->content_ptr) = *(int*)(addr);
			}
		}
	} else if(this->type > type) {
		if(this->type == INT || this->type == U_INT || this->type == LONG || this->type == U_LONG) {
			if(type == DOUBLE) {
				*(int*)(this->content_ptr) = *(double*)(addr);
			} else if(type == FLOAT) {
				*(int*)(this->content_ptr) = *(float*)(addr);
			} else {
				*(int*)(this->content_ptr) = *(int*)(addr);
			}
		} else if(this->type == U_SHORT || this->type == SHORT) {
			if(type == DOUBLE) {
				*(short*)(this->content_ptr) = *(double*)(addr);
			} else if(type == FLOAT) {
				*(short*)(this->content_ptr) = *(float*)(addr);
			} else {
				*(short*)(this->content_ptr) = *(short*)(addr);
			}
		} else if(this->type == U_CHAR || this->type == CHAR) {
			if(type == DOUBLE) {
				*(char*)(this->content_ptr) = *(double*)(addr);
			} else if(type == FLOAT) {
				*(char*)(this->content_ptr) = *(float*)(addr);
			} else {
				*(char*)(this->content_ptr) = *(char*)(addr);
			}
		} else if(this->type == FLOAT) {
			*(float*)(this->content_ptr) = *(double*)(addr);
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
		this->convert(addr, type);
	}
}

varity_info& varity_info::operator=(const varity_info& source)
{
	if(!this->size) {
		type = source.type;
		size = sizeof_type[type];
		this->apply_space();
		memcpy(this->content_ptr, source.content_ptr, size);
	} else {
		this->convert(source.content_ptr, source.type);
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
varity_info& varity_info::operator=(int source) 
{
	create_from_c_varity(&source, INT);
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
	if(this->content_ptr) {
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
	if(this->content_ptr) {
		vfree(this->content_ptr);
		this->content_ptr = 0;
	}
	if(this->name) {
		vfree(this->name);
		this->name = 0;
	}
}
void varity_info::echo(void)
{
	if(en_echo) {
		varity_info tmp;
		if(this->type == INT || this->type == LONG || this->type == SHORT || this->type == CHAR || this->type == U_SHORT || this->type == U_CHAR) {
			tmp.type = INT; tmp.size = sizeof_type[tmp.type]; tmp.apply_space();
			tmp = *this;
			debug("%s = %d\n",this->name, *(int*)this->content_ptr);
		} else if(this->type == U_INT || this->type == U_LONG || this->type == U_SHORT || this->type == U_CHAR)
			debug("%s = %lu\n",this->name, *(unsigned int*)this->content_ptr);
		else if(this->type == DOUBLE)
			debug("%s = %f\n",this->name, *(double*)this->content_ptr);
		else if(this->type == FLOAT)
			debug("%s = %f\n",this->name, *(float*)this->content_ptr);
	}
}

int varity::declare(int scope_flag, char* name, int type, uint size)
{//scope_flag = 0:global; 1: local; 2:analysis_tmp
	int ret;
	varity_info* varity_ptr;
	stack* varity_stack;
	if(!scope_flag) {
		if(this->global_varity_stack->find(name))
			return VARITY_DUPLICATE;
	}
	if(!scope_flag)
		varity_stack = global_varity_stack;
	else if(scope_flag == 1)
		varity_stack = local_varity_stack;
	else
		varity_stack = analysis_varity_stack;
	varity_ptr = (varity_info*)varity_stack->get_current_ptr();
	varity_info::init_varity(varity_ptr, name, type, size);
	ret = varity_ptr->apply_space();
	if(ret)
		return VARITY_NOSPACE;
	varity_stack->push();
	return 0;
}

int varity::declare_analysis_varity(int type, uint size, char* ret_name, varity_info** varity_ptr)
{
	int ret;
	char tmp_varity_name[3];
	sprintf(tmp_varity_name, "%c%c", TMP_VAIRTY_PREFIX, 128 + this->cur_analysis_varity_count++);
	strcpy(ret_name, tmp_varity_name);
	ret = this->declare(2, tmp_varity_name, type, size);
	*varity_ptr = (varity_info*)this->analysis_varity_stack->find(ret_name);
	return ret;
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