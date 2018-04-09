#pragma once
#ifndef DATA_STRUCTURE
#define DATA_STRUCTURE

#include "type.h"
#include "config.h"
#include "cstdlib.h"

class element {
protected:
	char*	name;
public:
	inline char* get_name(void) {return name;}
};

class data_struct {
protected:
	ushort count;
	ushort element_size;
	uint length;
	void* bottom_addr;
public:
	inline void* get_base_addr(void) {return this->bottom_addr;}
	inline int is_full(void) {return length==count?1:0;}
	inline uint get_count(void) {return this->count;}
};

class round_queue: public data_struct {
	uint wptr;
	uint rptr;
public:
	friend class c_interpreter;
	round_queue();
	round_queue(uint, void*);
	void init(uint count, uint element_size = 1);
	inline void set_base(void* addr) {this->bottom_addr = addr;}
	inline void set_length(uint len) {this->length = len;}
	inline void set_element_size(uint len) {this->element_size = len;}
	int write(const void*, uint);
	int read(void*, uint);
	int readline(char*);
	void content_reset(void) {wptr = rptr = count = 0;}
	uint get_wptr(void) {return wptr;}
	char* readline(int&);
};

class stack: public data_struct {//TODO:检查派生类的重写是否必要
protected:
	uint top;
public:
	void push(void*);
	void push(void) {this->top += this->element_size; this->count++;}
	void* pop(void);
	virtual void* find(char*);
	inline void set_base(void* addr) {this->bottom_addr = addr;}
	inline void set_count(int count) {this->count = count; this->top = count * this->element_size;}
	//inline void set_length(uint len) {this->length = len;}
	void empty(void) {kmemset(this->bottom_addr, 0, this->count * this->element_size); this->top = 0; this->count = 0;}
	stack();
	stack(int esize, void* base_addr, int capacity);
	void init(int esize, int capacity);
	void init(int, void*, int);
	void* get_current_ptr(void) {return (char*)this->bottom_addr + top;}
	void* get_lastest_element(void) {return this->count? (char*)get_current_ptr() - this->element_size: 0;}
	void* visit_element_by_index(int);
	void del_element_to(int count) {while(this->count > count) this->pop();}
};

class indexed_stack: public stack {
	int current_depth;
	uint index_table[MAX_STACK_INDEX];
	uint offset_table[MAX_STACK_INDEX];
	uint offset;
public:
	friend class c_interpreter;
	friend class varity;
	virtual void* find(char*);
	void *f_find(char*);
	void init(int esize, void* base_addr, int capacity);
	inline void reset(void) {kmemset(this->bottom_addr, 0 , this->top); this->count = 0; this->top = 0; this->offset = 0;}
	inline void endeep(void) {index_table[++current_depth] = this->count; offset_table[current_depth] = offset;}
	inline void dedeep(void) {offset = offset_table[current_depth]; index_table[current_depth--] = 0;}
};

typedef struct node_s {
	struct node_s *left;
	struct node_s *right;
	void *value;
	void link_reset(void) {this->left = this->right = 0;}
	void middle_visit(void);
} node;

class list_stack {
	node head;
	node tail;
	int count;
public:
	list_stack(void) {this->reset();}
	node* find_str_val(char *str);
	void push(node* obj) {obj->right = &tail; obj->left = tail.left; obj->left->right = obj; obj->right->left = obj; count++;}//TODO:中断调用时要加锁
	node *pop(void) {if(!this->get_count())return 0; node* ret = tail.left; tail.left = ret->left; ret->left->right = &tail; count--; return ret;}
	void reset(void) {this->head.right = &tail; this->tail.left = &head; this->count = 0;}
	int get_count(void) {return count;}
	node* get_head(void) {return &head;}
	node* get_lastest_element(void) {return tail.left;}
};

typedef struct varity_type_stack_s {
	int count;
	char arg_count[MAX_VARITY_TYPE_COUNT];
	void *type_info_addr[MAX_VARITY_TYPE_COUNT];
	int find(char arg_count, void *type_info_addr);
} varity_type_stack_t;
#endif
