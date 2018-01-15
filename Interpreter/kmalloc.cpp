#include <stdio.h>
#include <string.h>
#include "config.h"

#define ALIGN_BYTE 16
typedef unsigned int uint;
typedef struct head_head_s{
	uint size;
	head_head_s* next;
	head_head_s* last;
	uint isused;
}head_t;

static char heap[HEAPSIZE];
char* heapbase = heap;

inline uint align_size(uint size)
{
	return size + (size % ALIGN_BYTE == 0 ? 0 : (ALIGN_BYTE - size % ALIGN_BYTE));
}

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
	tsize = align_size(size);
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

int kfree(void *ptr) {
	head_t *headptr = (head_t*)ptr;
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

void* krealloc(void *ptr, uint size)
{
	head_t *headptr = (head_t*)ptr;
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
				new_next_ptr->size = (uint)next_headptr - (uint)new_next_ptr - sizeof(head_t);//����˳���ϸ񰴽ṹ�嶨��˳��
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
			if(next_headptr->isused == 0 && next_headptr->size + sizeof(head_t) >= size - headptr->size) {//ʣ��ռ����
				new_next_ptr = (head_t*)((uint)headptr + sizeof(head_t) + align_size(size));
				if((uint)next_headptr->next - (uint)new_next_ptr > sizeof(head_t)) {//�ŵ���header
					new_next_ptr->isused = 0;//����˳���ϸ񰴽ṹ�嶨��˳����
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
			} else {//���µİ�������;TODO:�жϼ�����һ���տ��С�Ƿ񹻣����Ļ�ֱ�Ӱ�
				void *new_ptr = kmalloc(size);
				if(new_ptr) {
					memcpy(new_ptr, ptr, size);
					kfree(ptr);
				}
				return new_ptr;
			}
		} else {
			return 0;
		}
	}
}

void heapinit(void) {
	head_t *head;
	head = (head_t*)heapbase;
	head->size = HEAPSIZE - sizeof(head_t);
	head->isused = 0;
	head->last = 0;
	head->next = 0;
}
