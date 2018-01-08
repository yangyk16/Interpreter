#include <stdio.h>
#include "config.h"

static char heap[HEAPSIZE];
char* heapbase = heap;

typedef unsigned int uint;
typedef struct head_head_s{
	uint size;
	head_head_s* next;
	head_head_s* last;
	uint isused;
}head_t;

void* kmalloc(uint size) {
	head_t* head, *lasthead, *nexthead;
	uint tsize;
	head = (head_t*)heapbase;
	lasthead = head;
	for(;head->isused==1 || head->size < size;) {
		lasthead = head;
		head = head->next;
		if(head == NULL)
			return 0;
	}
	tsize = size + (size%16==0?0:(16 - size % 16));
	debug("tsize=%d\n", tsize + sizeof(head_t));
	nexthead = head->next;
	head->isused = 1;
	head->next = (head_t*)((uint)head + sizeof(head_t) + tsize);
	if(head->next) {
		head->next->size = head->size - tsize - sizeof(head_t);
		head->next->isused = 0;
		head->next->next = nexthead;
		head->next->last = head;
	}
	head->size = tsize;
	head->last = lasthead;
	if(head->last != head)
		head->last->next = head;
	return (void*)head;
}

int kfree(void* ptr) {
	head_t* headptr = (head_t*)ptr;
	if(headptr->isused == 0)
		return -1;
	if(headptr->next != 0) {
		if(headptr->next->isused == 0) {
			headptr->size += headptr->next->size + sizeof(head_t);
			headptr->next = headptr->next->next;
		}
	}
	if(headptr->last->isused == 0) {
		headptr->last->next = headptr->next;
		headptr->last->size += headptr->size + sizeof(head_t);
	} else {
		headptr->isused = 0;
	}
	return 0;
}

void heapinit(void) {
	head_t* head;
	head = (head_t*)heapbase;
	head->size = HEAPSIZE - sizeof(head_t);
	head->isused = 0;
	head->last = 0;
	head->next = 0;
}
