#include "macro.h"
#include "error.h"
int macro::declare(char* name, char** parameter, char* instead_str)
{
	macro_info* macro_node_ptr;
	if(macro_node_ptr = (macro_info*)this->macro_stack_ptr->find(name)) {
		warning("declare macro \"%s\" error: macro name duplicated\n", name);
		macro_node_ptr->init(name, parameter, instead_str);
	} else {
		macro_info macro_node;
		macro_node.init(name, parameter, instead_str);
		macro_stack_ptr->push(&macro_node);
	}
	return 0;
}

void macro::init(stack* stack_ptr)
{
	this->macro_stack_ptr = stack_ptr;
}

void macro_info::dispose(void)
{
	if(this->macro_instead_str)
		vfree(this->macro_instead_str);
}

void macro_info::init(char* name, char* parameter[3], char* instead_str)
{
	this->name = name;
	for(int i=0; i<3; i++)
		this->macro_arg_name[i] = parameter[i];
	this->macro_instead_str = instead_str;
}

int macro_info::find(char *name)
{
	for(int i=0; this->macro_arg_name[i] && i<MAX_MACRO_ARG_COUNT; i++) {
		if(!kstrcmp(this->macro_arg_name[i], name))
			return i;
	}
	return -1;
}