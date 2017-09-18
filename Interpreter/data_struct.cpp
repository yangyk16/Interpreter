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

indexed_stack::indexed_stack(int esize, void* base_addr)
{
	stack();
	this->element_size = esize;
	this->bottom_addr = base_addr;
}

round_queue::round_queue()
{
	this->wptr = this->rptr = 0;
}
round_queue::round_queue(uint length, void* base_addr)
{
	this->length = length;
	this->wptr = this->rptr = 0;
	this->bottom_addr = base_addr;
}

int round_queue::write(void* buf, uint size)
{
	if(size > length - count)
		size = length - count;
	if(size + wptr > this->length) {
		memcpy((char*)this->bottom_addr + wptr * this->element_size, buf, this->element_size * (this->length - this->wptr));
		memcpy((char*)this->bottom_addr, (char*)buf + wptr * this->element_size, this->element_size * (size + wptr - this->length));
	} else {
		memcpy((char*)this->bottom_addr + wptr * this->element_size, buf, this->element_size * size);
	}
	wptr = (wptr + size) % this->length;
	count += size;
	return size;
}

int round_queue::read(void* buf, uint size)
{
	if(size > count)
		size = count;
	if(rptr + size > this->length) {
		memcpy(buf, (char*)this->bottom_addr + this->element_size * rptr, this->element_size * (length - rptr));
		memcpy((char*)buf + (length - rptr) * this->element_size, this->bottom_addr, (size + rptr - this->length) * this->element_size);
	} else {
		memcpy(buf, (char*)this->bottom_addr + this->element_size * rptr, this->element_size * size);
	}
	rptr = (rptr + size) % this->length;
	count -= size;
	return size;
}

int round_queue::readline(char* buf)
{
	int i = 0;
	char* base = (char*)this->bottom_addr;
	while(1)
	{
		if(count > 0)
			buf[i++] = base[rptr];
		else 
			return i;
		rptr = (rptr + 1) % length;
		if(buf[i - 1] = '\n')
			return i;
	}
	return -1;
}