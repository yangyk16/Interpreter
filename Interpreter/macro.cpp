#include "macro.h"
#include "error.h"
int macro::declare(char* name, char** parameter, char* instead_str)
{
	macro_info* macro_node_ptr;
	if(macro_node_ptr = (macro_info*)this->macro_stack_ptr->find(name)) {
		warning("declare macro \"%s\" error: macro name duplicated\n", name);
	} else {
		macro_node_ptr = (macro_info*)macro_stack_ptr->get_current_ptr();
		macro_stack_ptr->push();
	}
	macro_node_ptr->init(name, parameter, instead_str);
	return 0;
}

void macro::init(stack* stack_ptr)
{
	this->macro_stack_ptr = stack_ptr;
}

void macro_info::init(char* name, char* parameter[3], char* instead_str)
{
	this->name = name;
	for(int i=0; i<3; i++)
		this->macro_arg_name[i] = parameter[i];
	this->macro_instead_str = instead_str;
}