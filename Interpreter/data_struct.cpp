#include "data_struct.h"
#include "cstdlib.h"
#include "string_lib.h"
#include "kmalloc.h"

static int typecmp(int count, PLATFORM_WORD *type1, PLATFORM_WORD *type2);
static int argcmp(stack *std, stack *src);

stack::stack()
{
	this->count = 0;
	this->top = 0;
	this->bottom_addr = 0;
}

void stack::dispose(void)
{
	vfree(this->bottom_addr);
	this->bottom_addr = 0;
}

void stack::init(int esize, int capacity)
{
	this->count = 0;
	this->top = 0;	
	this->element_size = esize;
	this->bottom_addr = dmalloc(esize * capacity, "stack base");
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
	if(this->is_full()) {
		this->length *= 2;
		this->bottom_addr = vrealloc(this->bottom_addr, this->element_size * this->length);
	}
	kmemcpy((char*)this->bottom_addr + this->top, eptr, this->element_size);
	this->top += this->element_size;
	this->count++;
}

void stack::push(void)
{
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

void* stack::m_find(void *mem)
{
	for(unsigned int i=0; i<this->count; i++) {
		void *element_mem = (char*)this->bottom_addr + i * this->element_size;
		if(!kmemcmp(element_mem, mem, this->element_size))
			return element_mem;
	}
	return NULL;
}

void* stack::find(const char* name)
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

void arg_stack_stack::init(void)
{
	stack::init(sizeof(stack*), 16);
	//stack::init
}

void arg_stack_stack::dispose(void)
{
	for(int i=0; i<this->count; i++) {
		stack *arg_stack = (stack*)*(PLATFORM_WORD*)(this->visit_element_by_index(i));
		arg_stack->dispose();
		vfree(arg_stack);
	}
	vfree(this->bottom_addr);
}

void* arg_stack_stack::find(stack* stack_ptr)
{
	int count = this->get_count();
	for(int i=0; i<count; i++) {
		stack *ret = (stack*)*((PLATFORM_WORD*)(this->visit_element_by_index(i)));
		if(!argcmp(ret, stack_ptr))
			return this->visit_element_by_index(i);
	}
	return NULL;
}

void indexed_stack::init(int esize, int capacity)
{
	this->element_size = esize;
	if(!this->bottom_addr)
		this->bottom_addr = dmalloc(esize * capacity, "indexed stack base");
	else
		this->bottom_addr = vrealloc(this->bottom_addr, esize * capacity);
	this->length = capacity;
	this->current_depth = 0;
	kmemset(this->bottom_addr, 0, this->length * this->element_size);
}

void* indexed_stack::find(const char* name)//TODO:考虑复用stack find合理性
{
	char* element_name;
	for(int i=this->count-1; i>=0; i--) {
		element_name = ((element*)((char*)this->bottom_addr + i * this->element_size))->get_name();
		if(!kstrcmp(element_name, name))
			return (char*)this->bottom_addr + i * this->element_size;
	}
	return NULL;
}

void* indexed_stack::f_find(const char *name)
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

void round_queue::dispose(void)
{
	vfree(this->bottom_addr);
	this->bottom_addr = 0;
}

void round_queue::init(uint count, uint element_size)
{
	this->length = count;
	this->element_size = element_size;
	if(!this->bottom_addr)
		this->bottom_addr = dmalloc(length * element_size, "round queue base");
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

void node::middle_visit(void (*cb)(void*))
{
	if(this->left)
		this->left->middle_visit(cb);
	cb(this->value);
	if(this->right)
		this->right->middle_visit(cb);
}

int varity_type_stack_t::del(char arg_count, unsigned long *type_info_addr)
{
	int pos = this->find(arg_count, type_info_addr);
	if(pos >= 0) {
		this->arg_count[pos] = this->arg_count[this->count - 1];
		this->type_info_addr[pos] = this->type_info_addr[this->count - 1];
		this->count--;
		return 0;
	}
	error("Varity type not exist, can't be delete\n");
	return -1;
}

static int argcmp(stack *std, stack *src)
{
	int count = std->get_count();
	if(count != src->get_count())
		return -1;
	for(int i=0; i<count; i++) {
		varity_info *stdarg = (varity_info*)std->visit_element_by_index(i);
		varity_info *srcarg = (varity_info*)src->visit_element_by_index(i);
		if(stdarg->get_complex_arg_count() != srcarg->get_complex_arg_count())
			return -1;
		if(typecmp(stdarg->get_complex_arg_count(), stdarg->get_complex_ptr(), srcarg->get_complex_ptr()))
			return -1;
	}
	return 0;
}

static int typecmp(int count, PLATFORM_WORD *type1, PLATFORM_WORD *type2)
{
	int j;
	for(j=count; j>=1; j--) {
		if(type1[j] == type2[j]) {
			if(GET_COMPLEX_TYPE(type1[j]) == COMPLEX_ARG) {
				j--;
				if(argcmp((stack*)type1[j], (stack*)type2[j]))
					break;
			}
		} else
			break;
	}
	if(j == 0)
		return 0;
	return 1;
}

int varity_type_stack_t::find(char arg_count, unsigned long *type_info_addr)
{
	int i;
	for(i=0; i<this->count; i++) {
		if(this->arg_count[i] >= arg_count) {
			if(!typecmp(arg_count, this->type_info_addr[i], type_info_addr))
				return i;
		}
	}
	return -1;
}

int varity_type_stack_t::push(char arg_count, unsigned long *type_info_addr)
{
	this->arg_count[count] = arg_count;
	this->type_info_addr[count] = type_info_addr;
	return this->push();
}

int varity_type_stack_t::push(void)
{
	this->count++;
	if(this->count == this->length) {
		this->length *= 2;
		this->arg_count = (char*)vrealloc(this->arg_count, this->length * sizeof(char));
		this->type_info_addr = (PLATFORM_WORD**)vrealloc(this->type_info_addr, this->length * sizeof(void*));
	}
	return 0;
}

void varity_type_stack_t::dispose(void)
{
	vfree(this->arg_count);
	vfree(this->type_info_addr);
	this->arg_count = 0;
	this->type_info_addr = 0;
}

int varity_type_stack_t::init(void)
{
	this->arg_count = (char*)dmalloc(MAX_VARITY_TYPE_COUNT, "varity type arg count base");
	this->type_info_addr = (PLATFORM_WORD**)dmalloc(MAX_VARITY_TYPE_COUNT * PLATFORM_WORD_LEN, "type info addr");
	return 0;
}

node* list_stack::find_val(char *str)
{
	node *ptr;
	for(ptr=this->head.right; ptr!=&this->tail; ptr=ptr->right) {
		if(!kstrcmp((const char*)ptr->value, str)) {
			return ptr;
		}
	}
	return 0;
}

int list_stack::del(node *obj)
{
	if(obj->left && obj->right) {
		obj->left->right = obj->right;
		obj->right->left = obj->left;
		this->count--;
	} else {
		return -1;
	}
}

char* strfifo::get_real_wptr(uint count, int &remain)
{
	void *block_base = this->bottom_addr;
	int block_count = count / this->length;
	for(int i=0; i<block_count; i++)
		block_base = ((str_list*)block_base)->next;
	remain = this->length * (block_count + 1) - count;
	return (char*)block_base + sizeof(str_list) + count - block_count * this->length;
}

char* strfifo::get_new_block(uint count)
{
	if(this->length >= count)
		return (char*)this->bottom_addr;
	else {
		void *block_base = this->bottom_addr;
		int block_count = count / this->length;
		for(int i=0; i<block_count-1; i++)
			block_base = ((str_list*)block_base)->next;
		return ((str_list*)block_base)->next = (char*)dmalloc(this->length + sizeof(str_list), "string fifo new block");
	}
}

char* strfifo::write(const char *str)
{
	int remain;
	char *wptr = this->get_real_wptr(this->count, remain);
	unsigned int len = kstrlen(str) + 1;
	this->count += len;
	if(remain >= len) {
		kmemcpy(wptr, str, len);
	} else {
		this->count += remain;
		wptr = this->get_new_block(this->count);
		kmemcpy(wptr, str, len);
	}
	return wptr;
}

void strfifo::dispose(void)
{
	if(this->bottom_addr)
		vfree(this->bottom_addr);
	this->bottom_addr = 0;
}

void strfifo::init(uint length)
{
	if(!this->bottom_addr)
		this->bottom_addr = dmalloc(length + sizeof(str_list), "string fifo base");
	this->element_size = 1;
	this->count = 0;
	this->length = length;
}

int strfifo::del(char* str)
{
	unsigned int len;
	if(str - (char*)this->bottom_addr < 0 || str - (char*)this->bottom_addr >= this->count)
		return -1;
	len = kstrlen(str) + 1;
	kmemmove(str, str + len, len);
	//this->wptr -= len;
	return 0;
}