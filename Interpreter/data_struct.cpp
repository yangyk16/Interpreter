#include "data_struct.h"
#include "varity.h"
#include "cstdlib.h"
#include "interpreter.h"
#include "string_lib.h"

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
	kmemset(this->bottom_addr, 0, this->length * this->element_size);
}

stack::stack(int esize, void* base_addr, int capacity)
{
	this->init(esize, base_addr, capacity);
}

void stack::push(void* eptr)
{
	if(this->is_full())
		return;
	kmemcpy((char*)this->bottom_addr + this->top, eptr, this->element_size);
	this->top += this->element_size;
	this->count++;
}

void* stack::pop(void)
{
	if(count == 0) {
		error("Empty stack pop.\n");
		return NULL;
	}
	this->top -= this->element_size;
	this->count--;
	return (void*)&((char*)(this->bottom_addr))[this->top];
}

void* stack::find(char* name)
{
	char* element_name;
	for(uint i=0; i<this->count; i++) {
		element_name = ((element*)((char*)this->bottom_addr + i * this->element_size))->get_name();
		if(!kstrcmp(element_name, name))
			return (char*)this->bottom_addr + i * this->element_size;
	}
	return NULL;
}

void* stack::visit_element_by_index(int index)
{
	return (char*)this->bottom_addr + index * this->element_size;
}

void indexed_stack::init(int esize, void* base_addr, int capacity)
{
	this->element_size = esize;
	this->bottom_addr = base_addr;
	this->length = capacity;
	this->current_depth = 0;
	kmemset(this->bottom_addr, 0, this->length * this->element_size);
}

void* indexed_stack::find(char* name)//TODO:考虑复用stack find合理性
{
	char* element_name;
	for(int i=this->count-1; i>=0; i--) {
		element_name = ((element*)((char*)this->bottom_addr + i * this->element_size))->get_name();
		if(!kstrcmp(element_name, name))
			return (char*)this->bottom_addr + i * this->element_size;
	}
	return NULL;
}

void* indexed_stack::f_find(char *name)
{
	char *element_name;
	for(uint i=this->index_table[this->current_depth]; i<this->count; i++) {
		element_name = ((element*)((char*)this->bottom_addr + i * this->element_size))->get_name();
		if(!kstrcmp(element_name, name))
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

int round_queue::write(const void* buf, uint size)
{
	if(size > length - count)
		size = length - count;
	if(size + wptr > this->length) {
		kmemcpy((char*)this->bottom_addr + wptr * this->element_size, buf, this->element_size * (this->length - this->wptr));
		kmemcpy((char*)this->bottom_addr, (char*)buf + wptr * this->element_size, this->element_size * (size + wptr - this->length));
	} else {
		kmemcpy((char*)this->bottom_addr + wptr * this->element_size, buf, this->element_size * size);
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
		kmemcpy(buf, (char*)this->bottom_addr + this->element_size * rptr, this->element_size * (length - rptr));
		kmemcpy((char*)buf + (length - rptr) * this->element_size, this->bottom_addr, (size + rptr - this->length) * this->element_size);
	} else {
		kmemcpy(buf, (char*)this->bottom_addr + this->element_size * rptr, this->element_size * size);
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
		if(count > 0) {
			buf[i++] = base[rptr];
			count--;
		} else {
			buf[i] = 0;
			return i;
		}
		rptr = (rptr + 1) % length;
		if(buf[i - 1] == '\n' || buf[i - 1] == 0) {
			buf[i] = 0;
			return i;
		}
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

void node::middle_visit(void)
{
	if(this->left)
		this->left->middle_visit();
	node_attribute_t *tmp = (node_attribute_t*)this->value;
	debug("%d ",tmp->node_type);
	if(tmp->node_type == TOKEN_NAME)
		debug("%s\n",tmp->value.ptr_value);
	else
		debug("%d\n",tmp->value.int_value);
	if(this->right)
		this->right->middle_visit();
}

int varity_type_stack_t::find(char arg_count, void *type_info_addr)
{
	int i;
	for(i=0; i<this->count; i++) {
		if(this->arg_count[i] >= arg_count) {
			int j;
			for(j=1; j<=arg_count; j++) {
				if(((PLATFORM_WORD*)this->type_info_addr[i])[j] != ((PLATFORM_WORD*)type_info_addr)[j])
					break;
			}
			if(j > arg_count)
				return i;
		}
	}
	return -1;
}

node* list_stack::find_str_val(char *str)
{
	node *ptr;
	for(ptr=this->head.right; ptr!=&this->tail; ptr=ptr->right) {
		if(!kstrcmp((const char*)ptr->value, str)) {
			return ptr;
		}
	}
	return 0;
}