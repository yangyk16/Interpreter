#include "data_struct.h"
#include <string>
using namespace std;

stack::stack()
{
	this->top = 0;
}

stack::stack(int esize, void* base_addr)
{
	stack();
	this->element_size = esize;
	this->bottom_addr = base_addr;
}

void stack::push(void* eptr)
{
	memcpy(this->bottom_addr, eptr, this->element_size);
	this->top += this->element_size;
}

void* stack::pop(void)
{
	this->top -= this->element_size;
	return (void*)((char*)(this->bottom_addr))[this->top];
}

void* stack::find(void*)
{
	return NULL;
}