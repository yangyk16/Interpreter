#pragma once
#ifndef VARITY_H
#define VARITY_H

#include "type.h"
#include "data_struct.h"

class varity_info:public element {
	int		type;
	uint	size;
};

class varity {
	stack* global_varity_stack;
	data_struct* local_varity_struct;
	uint current_stack_depth;
public:
	int declare(char* name, uint size);
	int undeclare();
};

#endif
