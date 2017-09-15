#pragma once
#ifndef VARITY_H
#define VARITY_H

#include "type.h"
#include "data_struct.h"

class varity_info:public element {
protected:
	int		type;
	uint	size;
public:
	varity_info(char*, int, uint);
	varity_info();
};

class varity {
	stack* all_varity_stack;
	data_struct* frame_varity_struct;
	uint current_stack_depth;
public:
	int declare(char* name, int type, uint size);
	int undeclare();
};

#endif
