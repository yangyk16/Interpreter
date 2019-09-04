#include "config.h"
#include "cstdlib.h"

#define ALIGN_BYTE 16
#define NULL 0
typedef unsigned int uint;
typedef struct head_head_s{
	uint size;
	head_head_s* next;
	head_head_s* last;
	uint isused;
}head_t;

static char heap[HEAPSIZE];
char* heapbase = heap;

static inline uint align_size(uint size)
{
	return size + (size % ALIGN_BYTE == 0 ? 0 : (ALIGN_BYTE - size % ALIGN_BYTE));
}

static void print_mempool(void)
{
	head_t *head_ptr;
	for(head_ptr = (head_t*)heapbase; head_ptr; head_ptr = head_ptr->next)
		debug("0x%x, %d, u=%d\n", head_ptr + 1, head_ptr->size, head_ptr->isused);
	debug("mempool print finish.\n");
}

extern "C" void heap_debug(void)
{
	head_t *begin = (head_t*)heapbase;
	while(begin) {
		kprintf("%08x,isused=%d,size=%d\n", begin, begin->isused, begin->size);
		begin = begin->next;
	}
}

extern "C" void* kmalloc(uint size) {
	head_t *head_ptr, *next_head_ptr, *new_next_ptr;
	uint tsize;
	head_ptr = (head_t*)heapbase;
	for(;head_ptr->isused==1 || head_ptr->size < size;) {
		head_ptr = head_ptr->next;
		if(head_ptr == NULL) {
			return 0;
		}
	}
	tsize = align_size(size);
	//debug("tsize=%d\n", tsize + sizeof(head_t));
	next_head_ptr = head_ptr->next;
	if(!next_head_ptr)
		next_head_ptr = (head_t*)&heap[HEAPSIZE];
	if((long)next_head_ptr - (long)head_ptr - sizeof(head_t) - tsize > 0) {
		new_next_ptr = (head_t*)((long)head_ptr + sizeof(head_t) + tsize);
		new_next_ptr->size = (uint)next_head_ptr - (uint)new_next_ptr - sizeof(head_t);
		new_next_ptr->last = head_ptr;
		new_next_ptr->next = head_ptr->next;
		new_next_ptr->isused = 0;
		new_next_ptr->next && (new_next_ptr->next->last = new_next_ptr);
		head_ptr->next = new_next_ptr;
		head_ptr->size = tsize;
	}
	
	head_ptr->isused = 1;
	////////////////////////////////////////
	return (void*)((char*)head_ptr + sizeof(head_t));
}

extern "C" int kfree(void *ptr) {
	if(!ptr)
		return -1;
	head_t *headptr = (head_t*)ptr - 1;
	if(headptr->isused == 0)
		return -1;
	if(headptr->next != 0) {
		if(headptr->next->isused == 0) {
			headptr->size += headptr->next->size + sizeof(head_t);
			headptr->next = headptr->next->next;
			headptr->next && (headptr->next->last = headptr);
		}
	}
	if((char*)headptr != heapbase && headptr->last->isused == 0) {
		headptr->last->next = headptr->next;
		headptr->next && (headptr->next->last = headptr->last);
		headptr->last->size += headptr->size + sizeof(head_t);
	} else {
		headptr->isused = 0;
	}
	return 0;
}

extern "C" void* krealloc(void *ptr, uint size)
{
	head_t *headptr = (head_t*)ptr - 1;
	head_t *next_headptr = headptr->next;
	head_t *new_next_ptr;
	if(headptr->isused == 0)
		return 0;
	if(size < headptr->size) {
		if(next_headptr && !next_headptr->isused) {
			new_next_ptr = (head_t*)((uint)headptr + sizeof(head_t) + align_size(size));
			new_next_ptr->size = next_headptr->size + (uint)next_headptr - (uint)headptr;
			new_next_ptr->next = next_headptr->next;
			new_next_ptr->last = headptr;
			new_next_ptr->isused = 0;
			headptr->next = new_next_ptr;
			headptr->size = align_size(size);
		} else {
			if(!next_headptr)
				next_headptr = (head_t*)&heap[HEAPSIZE];
			if((uint)next_headptr - (uint)headptr - sizeof(head_t) - align_size(size) > sizeof(head_t)) {
				new_next_ptr = (head_t*)((uint)headptr + sizeof(head_t) + align_size(size));
				new_next_ptr->size = (uint)next_headptr - (uint)new_next_ptr - sizeof(head_t);//操作顺序严格按结构体定义顺序
				new_next_ptr->last = headptr;
				new_next_ptr->next = headptr->next;
				new_next_ptr->isused = 0;
				headptr->next = new_next_ptr;
				headptr->size = align_size(size);
			} else {
			}
		}
		return ptr;
	} else {
		if(headptr->next != 0) {
			if(next_headptr->isused == 0 && next_headptr->size + sizeof(head_t) >= size - headptr->size) {//剩余空间放下
				new_next_ptr = (head_t*)((uint)headptr + sizeof(head_t) + align_size(size));
				if((uint)next_headptr->next - (uint)new_next_ptr > sizeof(head_t)) {//放得下header
					new_next_ptr->isused = 0;//操作顺序严格按结构体定义顺序倒序
					new_next_ptr->last = headptr;
					new_next_ptr->next = next_headptr->next;
					new_next_ptr->size = (uint)new_next_ptr->next - (uint)new_next_ptr - sizeof(head_t);
					headptr->next = new_next_ptr;
					headptr->size = align_size(size);
				} else {
					headptr->next = headptr->next->next;
					headptr->next->last = headptr;
					headptr->size = (uint)headptr->next - (uint)headptr - sizeof(head_t);
				}
				return headptr;
			} else {//开新的搬走数据;TODO:判断加上上一个空块大小是否够，够的话直接搬
				void *new_ptr = kmalloc(size);
				if(new_ptr) {
					kmemcpy(new_ptr, ptr, size);
					kfree(ptr);
				}
				return new_ptr;
			}
		} else {
			return 0;
		}
	}
}

extern "C" void heapinit(void) {
	head_t *head;
	head = (head_t*)heapbase;
	head->size = HEAPSIZE - sizeof(head_t);
	head->isused = 0;
	head->last = 0;
	head->next = 0;
}

void* dmalloc(unsigned int size, const char *info)
{
	unsigned int a_size = size&7?size+(8-(size&7)):size;
	void* ret = kmalloc(a_size);
	debug("malloc %x, %d, %s\n", ret, size, info);
	if(ret) {
		//size = size&7?size+(8-size&7):size;
		//debug("malloc %x, %d\n", ret, size);
		kmemset(ret, 0, size);
		return ret;
	} else {
		error("Fatal error: no space alloc\n");
		print_mempool();
		return NULL;
	}
}

void vfree(void *ptr)
{
	kfree(ptr);
	debug("free %x\n", ptr);
}

void* vrealloc(void* addr, unsigned int size)
{
	void *ret = krealloc(addr, size);
	debug("realloc %x to %x, %d\n", addr, ret, size);
	return ret;
}
