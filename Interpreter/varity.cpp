#include "varity.h"
#include "data_struct.h"
#include <stdlib.h>
#include <string.h>

#if PLATFORM_WORD_LEN == 4
const char type_key[13][19] = {"void", "double", "float", "unsigned long long", "long long", "unsigned long", "unsigned int", "long", "int", "unsigned short", "short", "unsigned char", "char"};
const char sizeof_type[] = {0, 8, 4, 8, 8, 4, 4, 4, 4, 2, 2, 1, 1};
#elif PLATFORM_WORD_LEN == 8
const char type_key[13][19] = {"void", "double", "float", "unsigned long long", "long long", "unsigned long", "long", "unsigned int", "int", "unsigned short", "short", "unsigned char", "char"};
const char sizeof_type[] = {0, 8, 4, 8, 8, 8, 8, 4, 4, 2, 2, 1, 1};
#endif

varity_info::varity_info()
{

}

varity_info::varity_info(char* name, int type, uint size)
{
	this->name = name;
	this->type = type;
	this->size = size;
}

varity_info::varity_info(char source) 
{
	type = 12;
	size = sizeof_type[type];
	this->apply_space();
	memcpy(this->content_ptr, &source, size);
}
varity_info::varity_info(unsigned char source) 
{
	type = 11; 
	size = sizeof_type[type];
	this->apply_space();
	memcpy(this->content_ptr, &source, size);
}
varity_info::varity_info(short source) 
{
	type = 10; 
	size = sizeof_type[type];
	this->apply_space();
	memcpy(this->content_ptr, &source, size);
}
varity_info::varity_info(unsigned short source) 
{
	type = 9; 
	size = sizeof_type[type];
	this->apply_space();
	memcpy(this->content_ptr, &source, size);
}
varity_info::varity_info(int source) 
{
	type = 8; 
	size = sizeof_type[type];
	this->apply_space();
	memcpy(this->content_ptr, &source, size);
}
varity_info::varity_info(unsigned int source) 
{
	type = 6; 
	size = sizeof_type[type];
	this->apply_space();
	memcpy(this->content_ptr, &source, size);
}
varity_info::varity_info(long long source) 
{
	type = 4; 
	size = sizeof_type[type];
	this->apply_space();
	memcpy(this->content_ptr, &source, size);
}
varity_info::varity_info(unsigned long long source)
{
	type = 3;
	size = sizeof_type[type];
	this->apply_space();
	memcpy(this->content_ptr, &source, size);
}
varity_info::varity_info(float source)
{
	type = 2;
	size = sizeof_type[type];
	this->apply_space();
	memcpy(this->content_ptr, &source, size);
}
varity_info::varity_info(double source)
{
	type = 1;
	size = sizeof_type[type];
	this->apply_space();
	memcpy(this->content_ptr, &source, size);
}

int varity_info::apply_space(void)
{
	this->content_ptr = malloc(this->size);
	if(this->content_ptr)
		return 0;
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
	name_addr = (char*)malloc(name_len + 1);
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