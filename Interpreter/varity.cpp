#include "varity.h"
#include "data_struct.h"

varity_info varity_node[MAX_VARITY_NODE];
stack varity_list(sizeof(varity_info), varity_node);

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
	all_varity_stack->push(&varity);
	return 0;
}

int varity::undeclare(void)
{
	return 0;
}