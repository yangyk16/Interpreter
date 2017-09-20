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
}

varity_info::varity_info(char* name, int type, uint size)
{
	this->name = name;
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
	if(this->content_ptr)
		vfree(this->content_ptr);
	this->content_ptr = vmalloc(this->size);
	if(this->content_ptr)
		return 0;
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

int varity::declare(bool global_flag, char* name, int type, uint size)
{
	int ret, name_len;
	char* name_addr;
	if(global_flag) {
		if(this->global_varity_stack->find(name))
			return VARITY_DUPLICATE;
	}
	name_len = strlen(name);
	name_addr = (char*)vmalloc(name_len + 1);
	strcpy(name_addr, name);
	varity_info varity(name_addr, type, size);
	ret = varity.apply_space();
	if(ret)
		return VARITY_NOSPACE;
	if(global_flag)
		global_varity_stack->push(&varity);
	else
		local_varity_stack->push(&varity);
	return 0;
}

int varity::undeclare(void)
{
	return 0;
}

varity::varity(stack* g_stack, indexed_stack* l_stack)
{
	this->global_varity_stack = g_stack;
	this->local_varity_stack = l_stack;
}