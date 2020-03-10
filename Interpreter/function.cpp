#include "function.h"
#include "config.h"
#include "error.h"
#include "varity.h"
#include "config.h"
#include "interpreter.h"
#include "cstdlib.h"
#include "global.h"
#include "string_lib.h"

int function_info::init(char* name, int flag)
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
	if(!(flag & FUNC_FLAG_PROTOTYPE)) {
		this->buffer = (char*)dmalloc(MAX_FUNCTION_LEN, "function code buffer");
		this->row_begin_pos = (unsigned int*)dmalloc(MAX_FUNCTION_LINE * sizeof(char*), "function row ptr");
#if DEBUG_EN
		this->row_code_ptr = (int*)dmalloc(MAX_FUNCTION_LINE * sizeof(int), "function row map mid code");
#endif
	}
	this->compile_func_flag = flag;
	this->variable_para_flag = 0;
	this->debug_flag = 1;
	return 0;
}

int function_info::init(const char *name, void* addr, char variable_arg_flag)
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
	this->func_addr = addr;
	this->compile_func_flag = 1;
	this->variable_para_flag = variable_arg_flag;
	//this->arg_list = arg_list;
	return ERROR_NO;
}

#if DEBUG_EN
int function_info::copy_local_varity_stack(indexed_stack *lvsp)
{
	varity_info *cur_varity_ptr;
	int total_varity_count = lvsp->get_count();
	void *bottom_addr = dmalloc(total_varity_count * sizeof(varity_info), "function local varity base");
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

int function_info::abort(void)
{
	if(this->buffer)
	vfree(this->buffer);
#if DEBUG_EN
	if(this->row_code_ptr)
		vfree(this->row_code_ptr);
	if(this->local_varity_stack.get_base_addr())
		vfree(this->local_varity_stack.get_base_addr());
#endif
	if(this->row_begin_pos)
		vfree(this->row_begin_pos);
	return 0;
}

int function_info::dispose(void)
{
	//if(this->buffer)
	//	vfree(this->buffer);
#if DEBUG_EN
	//if(this->row_code_ptr)
	//	vfree(this->row_code_ptr);
	//if(this->local_varity_stack.get_base_addr())
	//	vfree(this->local_varity_stack.get_base_addr());
#endif
	//if(this->row_begin_pos)
	//	vfree(this->row_begin_pos);
	/*if(this->arg_list) {
		destroy_varity_stack(this->arg_list);
	}*/
	void *code_addr = this->mid_code_stack.get_base_addr();
	if(code_addr && this->mem_blk_flag) {
		vfree(code_addr);
	}
	return 0;
}

int function_info::size_adapt(void)
{
	unsigned int size;
	this->data_size = this->mid_code_stack.get_count() * sizeof(mid_code) + make_align(this->wptr, 4);
	//vrealloc(this->mid_code_stack.get_base_addr(), this->mid_code_stack.get_count() * sizeof(mid_code));
#if DEBUG_EN
	this->data_size += this->row_line * sizeof(mid_code*) + this->local_varity_stack.get_count() * sizeof(varity_info);
	//vrealloc(this->row_code_ptr, this->row_line * sizeof(mid_code*));
#endif
	//vrealloc(this->buffer, this->wptr);
	//vrealloc(this->row_begin_pos, this->row_line * sizeof(unsigned int));
	char *function_data = (char*)dmalloc(this->data_size + this->row_line * sizeof(unsigned int), "all function data");
	size = this->mid_code_stack.get_count() * sizeof(mid_code);
	kmemcpy(function_data, this->mid_code_stack.get_base_addr(), size);
	vfree(this->mid_code_stack.get_base_addr());
	this->mid_code_stack.set_base(function_data);
	function_data += size;
	size = make_align(this->wptr, 4);
	kmemcpy(function_data, this->buffer, size);
	vfree(this->buffer);
	this->buffer = function_data;
#if DEBUG_EN
	function_data += size;
	size = this->local_varity_stack.get_count() * sizeof(varity_info);
	kmemcpy(function_data, this->local_varity_stack.get_base_addr(), size);
	vfree(this->local_varity_stack.get_base_addr());
	this->local_varity_stack.set_base(function_data);
	function_data += size;
	size = this->row_line * sizeof(int);
	kmemcpy(function_data, this->row_code_ptr, size);
	vfree(this->row_code_ptr);
	this->row_code_ptr = (int*)function_data;
#endif
	function_data += size;
	size = this->row_line * sizeof(unsigned int);
	kmemcpy(function_data, this->row_begin_pos, size);
	vfree(this->row_begin_pos);
	this->row_begin_pos = (unsigned int*)function_data;
	return ERROR_NO;
}

int function_info::save_sentence(char* str, uint len)
{
	this->row_begin_pos[row_line] = wptr;
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
	this->wptr = this->row_begin_pos[--row_line];
	return ERROR_NO;
}

#if DEBUG_EN
int function_info::print_line(int line)
{
	if(line < 0 || line > this->row_line)
		return -1;
	gdbout("%03d %s\n", line, this->buffer + row_begin_pos[line]);
	return 0;
}
#endif

void function::init(stack* function_stack_ptr)
{
	this->function_stack_ptr = function_stack_ptr;
}

int function::declare(char *name, int arg_count, PLATFORM_WORD *arg, int flag)
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
	function_node_ptr->init(name, flag);
	function_node_ptr->arg = arg;
	function_node_ptr->arg_count = arg_count;
	function_node_ptr->mem_blk_flag = 1;
	arg[0]++;
	if(!(flag & FUNC_FLAG_PROTOTYPE))
		function_node_ptr->mid_code_stack.init(sizeof(mid_code), MAX_MID_CODE_COUNT);
	this->current_node = function_node_ptr;
	function_stack_ptr->push();
	return 0;
}

int function::add_compile_func(const char *name, void *addr, int arg_cnt, PLATFORM_WORD *arg, char variable_arg_flag)
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
	function_node_ptr->init(name, addr, variable_arg_flag);
	function_node_ptr->arg_count = arg_cnt;
	function_node_ptr->arg = arg;
	arg[0]++;
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
