#include "data_struct.h"
#include "varity.h"
#include <string>
using namespace std;

stack::stack()
{
	this->count = 0;
	this->top = 0;
}

void stack::init(int esize, int capacity)
{
	this->count = 0;
	this->top = 0;	
	this->element_size = esize;
	this->bottom_addr = vmalloc(esize * capacity);
	this->length = capacity;
}

void stack::init(int esize, void* base_addr, int capacity)
{
	this->count = 0;
	this->top = 0;	
	this->element_size = esize;
	this->bottom_addr = base_addr;
	this->length = capacity;
	memset(this->bottom_addr, 0, this->length * this->element_size);
}

stack::stack(int esize, void* base_addr, int capacity)
{
	this->init(esize, base_addr, capacity);
}

void stack::push(void* eptr)
{
	if(this->is_full())
		return;
	memcpy((char*)this->bottom_addr + this->top, eptr, this->element_size);
	this->top += this->element_size;
	this->count++;
}

void* stack::pop(void)
{
	if(count == 0)
		return NULL;
	this->top -= this->element_size;
	this->count--;
	return (void*)&((char*)(this->bottom_addr))[this->top];
}

void* stack::find(char* name)
{
	char* element_name;
	for(uint i=0; i<this->count; i++) {
		element_name = ((element*)((char*)this->bottom_addr + i * this->element_size))->get_name();
		if(!strcmp(element_name, name))
			return (char*)this->bottom_addr + i * this->element_size;
	}
	return NULL;
}

void* stack::visit_element_by_index(int index)
{
	return (char*)this->bottom_addr + index * this->element_size;
}

indexed_stack::indexed_stack(int esize, void* base_addr, int capacity)
{
	stack();
	this->element_size = esize;
	this->bottom_addr = base_addr;
	this->length = capacity;
	this->current_depth = -1;
	memset(this->bottom_addr, 0, this->length * this->element_size);
}

void* indexed_stack::find(char* name)
{
	char* element_name;
	int visible_index = this->index_table[this->visible_depth];
	for(uint i=visible_index; i<this->count; i++) {
		element_name = ((element*)((char*)this->bottom_addr + i * this->element_size))->get_name();
		if(!strcmp(element_name, name))
			return (char*)this->bottom_addr + i * this->element_size;
	}
	return NULL;
}

round_queue::round_queue()
{
	this->wptr = this->rptr = 0;
}

void round_queue::init(uint count, uint element_size)
{
	this->length = count;
	this->element_size = element_size;
	this->bottom_addr = vmalloc(length * element_size);
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
		if(buf[i - 1] == '\n' || buf[i - 1] == 0)
			return i;
	}
	return -1;
}

char* round_queue::readline(int& len)
{
	char* base = (char*)this->bottom_addr;
	int ret = rptr;
	len = 0;
	while(1)
	{
		if(count > 0)
			len++;
		else {
			break;
		}
		rptr = (rptr + 1) % length;
		if(base[rptr - 1] == '\n') {
			len -= 1;
			break;
		}
	}
	return &((char*)bottom_addr)[ret];
}
#include "interpreter.h"
#include "string_lib.h"
void node::middle_visit(void)
{
	if(this->left)
		this->left->middle_visit();
	node_attribute_t *tmp = (node_attribute_t*)this->value;
	printf("%d ",tmp->node_type);
	if(tmp->node_type == TOKEN_NAME)
		printf("%s\n",tmp->value.ptr_value);
	else
		printf("%d\n",tmp->value.int_value);
	if(this->right)
		this->right->middle_visit();
}