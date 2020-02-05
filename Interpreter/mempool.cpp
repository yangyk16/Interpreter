#include "config.h"
#include "cstdlib.h"

#define MEMCHK_EN	1
#define ALIGN_BYTE	16
#define NULL 0
#define MEM_MAGIC	0x5A
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef struct head_head_s{
	uint size;
	head_head_s* next;
	head_head_s* last;
	uchar isused;
#if MEMCHK_EN
	uchar magic;
	ushort crc16;
#endif
}head_t;

static int heap[HEAPSIZE / sizeof(int)];
char* heapbase = (char*)heap;

#if MEMCHK_EN
int head_check(head_t* head_ptr)
{
	if(head_ptr->magic != MEM_MAGIC)
		return -1;
	return 0;
}
#endif
static inline uint align_size(uint size)
{
	return size + (size % ALIGN_BYTE == 0 ? 0 : (ALIGN_BYTE - size % ALIGN_BYTE));
}

static void print_mempool(void)
{
	head_t *head_ptr;
	for(head_ptr = (head_t*)heapbase; head_ptr; head_ptr = head_ptr->next)
		memdebug("0x%x, %d, u=%d\n", head_ptr + 1, head_ptr->size, head_ptr->isused);
	memdebug("mempool print finish.\n");
}

static int heap_self_check(void)
{
	head_t *head_ptr;
	for(head_ptr = ((head_t*)heapbase)->next; head_ptr; head_ptr = head_ptr->next) {
		if(head_ptr->last->next != head_ptr || head_ptr->next && head_ptr->next->last != head_ptr) {
			fatal("heap wrong.\n");
			return 1;
		}
		head_t *next_head = head_ptr->next ? head_ptr->next : (head_t*)(&heap + 1);
		if((char*)next_head - (char*)head_ptr != head_ptr->size + sizeof(head_t)) {
			fatal("heap wrong.\n");
			return 1;
		}
#if MEMCHK_EN
		if(head_check(head_ptr)) {
			fatal("heap wrong.\n");
			return 1;
		}
#endif
	}
	return 0;
}

extern "C" void heap_debug(void)
{
	head_t *begin = (head_t*)heapbase;
	if(heap_self_check() || 1) {
		while (begin) {
			kprintf("%08x,isused=%d,size=%d\n", begin + 1, begin->isused, begin->size);
			begin = begin->next;
		}
		memdebug("mempool print finish.\n");
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
		next_head_ptr = (head_t*)(&heap + 1);
	if((long)next_head_ptr - (long)head_ptr - sizeof(head_t) - tsize > 0) {
		new_next_ptr = (head_t*)((long)head_ptr + sizeof(head_t) + tsize);
		new_next_ptr->size = (uint)next_head_ptr - (uint)new_next_ptr - sizeof(head_t);
		new_next_ptr->last = head_ptr;
		new_next_ptr->next = head_ptr->next;
#if MEMCHK_EN
		new_next_ptr->magic = MEM_MAGIC;
#endif
		new_next_ptr->isused = 0;
		new_next_ptr->next && (new_next_ptr->next->last = new_next_ptr);
		head_ptr->next = new_next_ptr;
		head_ptr->size = tsize;
#if MEMCHK_EN
		head_ptr->magic = MEM_MAGIC;
#endif
	}
	
	head_ptr->isused = 1;
	////////////////////////////////////////
	return (void*)((char*)head_ptr + sizeof(head_t));
}

extern "C" int kfree(void *ptr) {
	if(!ptr) {
		fatal("Fatal error:free null ptr\n");
		return -1;
	}
	head_t *headptr = (head_t*)ptr - 1;
#if MEMCHK_EN
	if (head_check(headptr)) {
		fatal("Fatal error:not valid heap mem\n");
		return -2;
	}
#endif
	if(headptr->isused == 0) {
		fatal("Fatal error:free memory freed\n");
		return -1;
	}
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
#if MEMCHK_EN
	if(head_check(headptr)) {
		fatal("Fatal error:not valid heap mem\n");
		return 0;
	}
#endif
	if(headptr->isused == 0) {
		fatal("Fatal error:realloc freed memory");
		return 0;
	}
	if(size < headptr->size) {
		if(next_headptr && !next_headptr->isused) {
			new_next_ptr = (head_t*)((uint)headptr + sizeof(head_t) + align_size(size));
#if MEMCHK_EN
			new_next_ptr->magic = MEM_MAGIC;
#endif
			new_next_ptr->isused = 0;//TODO: 改成和202行同样顺序，严格倒序
			new_next_ptr->size = next_headptr->size + (uint)headptr->next - (uint)new_next_ptr;
			new_next_ptr->next = next_headptr->next;
			new_next_ptr->next && (new_next_ptr->next->last = new_next_ptr);
			new_next_ptr->last = headptr;
			headptr->next = new_next_ptr;
			headptr->size = align_size(size);
		} else {
			if(!next_headptr)
				next_headptr = (head_t*)(&heap + 1);
			if((uint)next_headptr - (uint)headptr - sizeof(head_t) - align_size(size) > sizeof(head_t)) {
				new_next_ptr = (head_t*)((uint)headptr + sizeof(head_t) + align_size(size));
				new_next_ptr->size = (uint)next_headptr - (uint)new_next_ptr - sizeof(head_t);//操作顺序严格按结构体定义顺序
				new_next_ptr->last = headptr;
				new_next_ptr->next = headptr->next;
				new_next_ptr->next && (new_next_ptr->next->last = new_next_ptr);
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
#if MEMCHK_EN
					new_next_ptr->magic = MEM_MAGIC;
#endif					
					new_next_ptr->isused = 0;//操作顺序严格按结构体定义顺序倒序
					new_next_ptr->last = headptr;
					new_next_ptr->next = next_headptr->next;
					new_next_ptr->next && (new_next_ptr->next->last = new_next_ptr);
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
	head->size = sizeof(heap) - sizeof(head_t);
	head->isused = 0;
	head->last = 0;
	head->next = 0;
#if MEMCHK_EN
	head->magic = MEM_MAGIC;
#endif
}

void* dmalloc(unsigned int size, const char *info)
{
	unsigned int a_size = size&7?size+(8-(size&7)):size;
	void* ret = kmalloc(a_size);
#if INTERPRETER_DEBUG
	memdebug("malloc %x, %d, %s\n", ret, size, info);
#else
	memdebug("malloc %x, %d\n", ret, size);
#endif
	//heap_debug();
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
	memdebug("free %x\n", ptr);
	//heap_debug();
}

void* vrealloc(void* addr, unsigned int size)
{
	void *ret = krealloc(addr, size);
	memdebug("realloc %x to %x, %d\n", addr, ret, size);
	//heap_debug();
	return ret;
}
