#include "varity.h"
#include "data_struct.h"

varity_info::varity_info()
{

}

varity_info::varity_info(char* name, int type, uint size)
{
	this->name = name;
	this->type = type;
	this->size = size;
}

int varity::declare(char* name, int type, uint size)
{
	varity_info varity(name, type, size);
	//all_varity_stack->push(&varity);
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