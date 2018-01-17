#include "function.h"
#include "config.h"
#include "error.h"
#include "varity.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "interpreter.h"
#include "cstdlib.h"

int function_info::init(char* name, stack* arg_list)
{//TODO: add malloc fail action.
	int name_len = strlen(name);
	this->name = (char*)vmalloc(name_len + 1);
	strcpy(this->name, name);
	this->buffer = (char*)vmalloc(MAX_FUNCTION_LEN);
	this->row_begin_pos = (char**)vmalloc(MAX_FUNCTION_LINE * sizeof(char*));
	this->row_len = (int*)vmalloc(MAX_FUNCTION_LINE * sizeof(int));
	this->arg_list = arg_list;
	this->compile_func_flag = 0;
	this->variable_para_flag = 0;
	return 0;
}

int function_info::init(char *name, void* addr, stack *arg_list, char variable_arg_flag)
{
	int name_len = strlen(name);
	this->name = (char*)vmalloc(name_len + 1);
	strcpy(this->name, name);
	this->func_addr = addr;
	this->compile_func_flag = 1;
	this->variable_para_flag = variable_arg_flag;
	this->arg_list = arg_list;
	return ERROR_NO;
}

int function_info::reset(void)
{
	if(this->name)
		vfree(this->name);
	if(this->buffer)
		vfree(this->buffer);
	if(this->row_begin_pos)
		vfree(this->row_begin_pos);
	if(this->row_len)
		vfree(this->row_len);
	if(this->arg_list) {
		vfree(this->arg_list->visit_element_by_index(0));
		vfree(this->arg_list);
	}
	return 0;
}

int function_info::size_adapt(void)
{
	vrealloc(this->mid_code_stack.get_base_addr(), this->mid_code_stack.get_count() * sizeof(mid_code));
	vrealloc(this->buffer, this->wptr);
	vfree(this->row_len);
	vrealloc(this->row_begin_pos, this->row_line);
	return ERROR_NO;
}

int function_info::save_sentence(char* str, uint len)
{
	this->row_len[row_line] = len;
	this->row_begin_pos[row_line] = this->buffer + wptr;
	this->row_line++;
	kmemcpy(this->buffer + wptr, str, len);
	this->buffer[wptr + len] = '\0';//\n
	wptr += len + 1;
	return 0;
}

function::function(stack* function_stack_ptr)
{
	this->function_stack_ptr = function_stack_ptr;
}

int function::declare(char* name, stack* arg_list)
{
	function_info* function_node_ptr;
	if(this->function_stack_ptr->find(name)) {
		error("declare function \"%s\" error: function name duplicated\n", name);
		return ERROR_FUNC_DUPLICATE;
	}
	if(function_stack_ptr->is_full()) {
		error("declare function \"%s\" error: varity count reach max\n", name);
		return ERROR_FUNC_COUNT_MAX;
	}
	function_node_ptr = (function_info*)function_stack_ptr->get_current_ptr();
	function_node_ptr->init(name, arg_list);
	function_node_ptr->mid_code_stack.init(sizeof(mid_code), MAX_MID_CODE_COUNT);
	this->current_node = function_node_ptr;
	function_stack_ptr->push();
	return 0;
}

int function::add_compile_func(char *name, void *addr, stack *arg_list, char variable_arg_flag)
{
	function_info* function_node_ptr;
	if(this->function_stack_ptr->find(name)) {
		error("Add function \"%s\" error: function name duplicated\n", name);
		return ERROR_FUNC_DUPLICATE;
	}
	if(function_stack_ptr->is_full()) {
		error("Add function \"%s\" error: varity count reach max\n", name);
		return ERROR_FUNC_COUNT_MAX;
	}
	function_node_ptr = (function_info*)function_stack_ptr->get_current_ptr();
	function_node_ptr->init(name, addr, arg_list, variable_arg_flag);
	function_stack_ptr->push();
	return ERROR_NO;
}

int function::save_sentence(char* str, uint len)
{
	return this->current_node->save_sentence(str, len);
}