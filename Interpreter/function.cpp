#include "function.h"
#include "config.h"
#include "error.h"
#include "varity.h"
#include "config.h"
#include "interpreter.h"
#include "cstdlib.h"
#include "global.h"

int function_info::init(char* name, stack* arg_list)
{//TODO: add malloc fail action.
	int name_len = kstrlen(name);
	string_info *string_ptr;
	string_ptr = (string_info*)name_stack.find(name);
	if(!string_ptr) {
		string_info tmp;
		this->name = name_fifo.write(name);
		tmp.set_name(this->name);
		name_stack.push(&tmp);
	} else {
		this->name = string_ptr->get_name();
	}
	//this->name = (char*)dmalloc(name_len + 1, "");
	//kstrcpy(this->name, name);
	this->buffer = (char*)dmalloc(MAX_FUNCTION_LEN, "");
	this->row_begin_pos = (char**)dmalloc(MAX_FUNCTION_LINE * sizeof(char*), "");
	this->row_len = (int*)dmalloc(MAX_FUNCTION_LINE * sizeof(int), "");
#if DEBUG_EN
	this->row_code_ptr = (mid_code**)dmalloc(MAX_FUNCTION_LINE * sizeof(mid_code*), "");
#endif
	this->arg_list = arg_list;
	this->compile_func_flag = 0;
	this->variable_para_flag = 0;
	return 0;
}

int function_info::init(char *name, void* addr, stack *arg_list, char variable_arg_flag)
{
	int name_len = kstrlen(name);
	string_info *string_ptr;
	string_ptr = (string_info*)name_stack.find(name);
	if(!string_ptr) {
		string_info tmp;
		this->name = name_fifo.write(name);
		tmp.set_name(this->name);
		name_stack.push(&tmp);
	} else {
		this->name = string_ptr->get_name();
	}
	//this->name = (char*)dmalloc(name_len + 1, "");
	//kstrcpy(this->name, name);
	this->func_addr = addr;
	this->compile_func_flag = 1;
	this->variable_para_flag = variable_arg_flag;
	this->arg_list = arg_list;
	return ERROR_NO;
}

#if DEBUG_EN
int function_info::copy_local_varity_stack(indexed_stack *lvsp)
{
	varity_info *cur_varity_ptr;
	int total_varity_count = lvsp->get_count();
	void *bottom_addr = dmalloc(total_varity_count * sizeof(varity_info), "");
	kmemcpy(&this->local_varity_stack, lvsp, sizeof(indexed_stack));
	this->local_varity_stack.set_base(bottom_addr);
	for(int i=0; i<total_varity_count; i++) {
		varity_info *copied_varity_ptr = (varity_info*)lvsp->visit_element_by_index(i);
		varity_info *copying_varity_ptr = (varity_info*)this->local_varity_stack.visit_element_by_index(i);
		kmemcpy(copying_varity_ptr, copied_varity_ptr, sizeof(varity_info));
		//char *name_ptr = (char*)dmalloc(kstrlen(copied_varity_ptr->get_name()) + 1, "");
		//kstrcpy(name_ptr, copying_varity_ptr->get_name());
		copying_varity_ptr->set_name(copying_varity_ptr->get_name());
	}
	return ERROR_NO;
}
#endif

int function_info::reset(void)
{
	if(this->name)
		vfree(this->name);
	if(this->buffer)
		vfree(this->buffer);
#if DEBUG_EN
	if(this->row_code_ptr)
		vfree(this->row_code_ptr);
#endif
	if(this->row_begin_pos)
		vfree(this->row_begin_pos);
	if(this->row_len)
		vfree(this->row_len);
	if(this->arg_list) {
		destroy_varity_stack(this->arg_list);
	}
	void *code_addr = this->mid_code_stack.get_base_addr();
	if(code_addr) {
		vfree(code_addr);
	}
	return 0;
}

int function_info::size_adapt(void)
{
	vrealloc(this->mid_code_stack.get_base_addr(), this->mid_code_stack.get_count() * sizeof(mid_code));
#if DEBUG_EN
	vrealloc(this->row_code_ptr, this->row_line * sizeof(mid_code*));
#endif
	vrealloc(this->buffer, this->wptr);
	vfree(this->row_len);
	vrealloc(this->row_begin_pos, this->row_line * sizeof(char*));
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

int function_info::destroy_sentence(void)
{
	if(!row_line)
		return ERROR_EMPTY_FIFO;
	this->wptr = this->row_begin_pos[--row_line] - this->buffer;
	return ERROR_NO;
}

#if DEBUG_EN
int function_info::print_line(int line)
{
	if(line < 0 || line > this->row_line)
		return -1;
	gdbout("%03d %s\n", line, row_begin_pos[line]);
	return 0;
}
#endif

void function::init(stack* function_stack_ptr)
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

int function::destroy_sentence(void)
{
	return this->current_node->destroy_sentence();
}
