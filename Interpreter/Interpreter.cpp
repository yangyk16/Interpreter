#include "interpreter.h"
#include "operator.h"
#include "string_lib.h"
#include "error.h"
#include "varity.h"
#include "data_struct.h"
#include "macro.h"
#include "cstdlib.h"
#include "kmalloc.h"
#include "gdb.h"
#include "global.h"

language_elment_space_t c_interpreter::language_elment_space;
compile_info_t c_interpreter::compile_info;
compile_function_info_t c_interpreter::compile_function_info;
compile_string_info_t c_interpreter::compile_string_info;
compile_varity_info_t c_interpreter::compile_varity_info;
compile_struct_info_t c_interpreter::compile_struct_info;
preprocess_info_t c_interpreter::preprocess_info;
int c_interpreter::cstdlib_func_count;
round_queue token_fifo;

char non_seq_key[][7] = {"", "if", "switch", "else", "for", "while", "do"};
char ctl_key[][9] = {"break", "continue", "goto", "return"};
const char non_seq_key_len[] = {0, 2, 6, 4, 3, 5, 2};
char opt_str[43][4] = {"<<=",">>=","->","++","--","<<",">>",">=","<=","==","!=","&&","||","/=","*=","%=","+=","-=","&=","^=","|=","[","]","(",")",".","-","~","*","&","!","/","%","+",">","<","^","|","?",":","=",",",";"};
char opt_str_len[] = {3,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
char opt_prio[] ={14,14,1,2,2,5,5,6,6,7,7,11,12,14,14,14,14,14,14,14,14,1,1,1,17,1,4,2,3,8,2,3,3,4,6,6,9,10,13,13,14,15,16};
char opt_number[] = {2,2,2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,2,2,2,1,1,1,1,1,1,1};
char tmp_varity_name[MAX_A_VARITY_NODE][3];
char link_varity_name[MAX_A_VARITY_NODE][3];

int c_interpreter::list_to_tree(node* tree_node, list_stack* post_order_stack)
{
	node *last_node;
	node_attribute_t *last_node_attribute;
	int ret;
	if(!tree_node->right) {
		last_node = (node*)post_order_stack->pop();
		if(!last_node) {
			tip_wrong(((node_attribute_t*)tree_node->value)->pos);
			error("Operand insufficient.\n");
			return ERROR_OPERAND_LACKED;
		}
		last_node->link_reset();
		last_node_attribute = (node_attribute_t*)last_node->value;
		tree_node->right = last_node;
		if(last_node_attribute->node_type == TOKEN_OPERATOR) {
			if(last_node_attribute->data == OPT_TYPE_CONVERT && ((node_attribute_t*)tree_node->value)->data == OPT_SIZEOF) {
				return ERROR_NO;
			}
			ret = list_to_tree(last_node, post_order_stack);
			if(ret)
				return ret;
		} else if(last_node_attribute->node_type == TOKEN_NAME || last_node_attribute->node_type == TOKEN_CONST_VALUE || last_node_attribute->node_type == TOKEN_STRING) {
		} else {
			error("Invalid key word.\n");
			return ERROR_INVALID_KEYWORD;
		}
		if(((node_attribute_t*)tree_node->value)->node_type == TOKEN_OPERATOR && opt_number[((node_attribute_t*)tree_node->value)->data] == 1) {//单目运算符
			return ERROR_NO;
		}
		if(((node_attribute_t*)tree_node->value)->node_type == TOKEN_OPERATOR && ((node_attribute_t*)tree_node->value)->data == OPT_CALL_FUNC) {
			function_info *function_ptr = this->function_declare->find(((node_attribute_t*)tree_node->value - 1)->value.ptr_value);
			unsigned int arg_count;
			if(!function_ptr) {
				varity_info *varity_info_ptr = this->varity_declare->find(((node_attribute_t*)tree_node->value - 1)->value.ptr_value);
				if(!varity_info_ptr) {
					error("Function %s not found.\n", ((node_attribute_t*)tree_node->value - 1)->value.ptr_value);
					return ERROR_NO_FUNCTION;
				} else {
					arg_count = ((stack*)varity_info_ptr->get_complex_ptr()[2])->get_count();
				}
			} else {
				arg_count = function_ptr->arg_list->get_count() - 1;
			}
			if(arg_count == 0) {//无参数函数
				tree_node->left = last_node;
				tree_node->right = 0;
				return ERROR_NO;
			}
		}
	}
	if(!tree_node->left) {
		last_node = (node*)post_order_stack->pop();
		if(!last_node) {
			tip_wrong(((node_attribute_t*)tree_node->value)->pos);
			error("Operand insufficient.\n");
			return ERROR_OPERAND_LACKED;
		}
		last_node->link_reset();
		last_node_attribute = (node_attribute_t*)last_node->value;
		tree_node->left = last_node;
		if(last_node_attribute->node_type == TOKEN_OPERATOR) {
			ret = list_to_tree(last_node, post_order_stack);
			if(ret)
				return ret;
		} else if(last_node_attribute->node_type == TOKEN_NAME || last_node_attribute->node_type == TOKEN_CONST_VALUE || last_node_attribute->node_type == TOKEN_STRING) {
		} else {
			error("Invalid key word.\n");
			return ERROR_INVALID_KEYWORD;
		}
	}
	return ERROR_NO;
}

int c_interpreter::mem_rearrange(void)
{
	int count;
	int i;
	char *base;
	unsigned int copy_len;
	unsigned int name_total_len = 0;
	unsigned int source_total_len = 0;
	unsigned int code_total_len = 0;
	count = this->function_declare->function_stack_ptr->get_count();
	function_info *function_ptr = (function_info*)this->function_declare->function_stack_ptr->get_base_addr();
	for(i=0; i<count; i++) {
		name_total_len += kstrlen(function_ptr[i].get_name()) + 1;
		source_total_len += function_ptr[i].wptr;
		code_total_len += function_ptr[i].mid_code_stack.get_count() * sizeof(mid_code);
	}
	base = (char*)kmalloc(sizeof(unsigned int) + sizeof(function_info) * count + name_total_len + source_total_len + code_total_len);
	U_INT_VALUE(base) = name_total_len + source_total_len + code_total_len;
	base += sizeof(unsigned int);
	for(i=0; i<count; i++) {
		copy_len = sizeof(function_info);
		kmemcpy(base, &function_ptr[i], copy_len);
		base += copy_len;
	}
	for(i=0; i<count; i++) {
		copy_len = function_ptr[i].mid_code_stack.get_count() * sizeof(mid_code);
		kmemcpy(base, function_ptr[i].mid_code_stack.get_base_addr(), copy_len);
		base += copy_len;
	}
	for(i=0; i<count; i++) {
		copy_len = kstrlen(function_ptr[i].get_name()) + 1;
		kstrcpy(base, function_ptr[i].get_name());
		base += copy_len;
	}
	for(i=0; i<count; i++) {
		copy_len = function_ptr[i].wptr;
		kmemcpy(base, function_ptr[i].buffer, copy_len);
		base += copy_len;
	}
	//%%%%%function rearrange end%%%%%%%%%%%%//
	count = string_stack.get_count();
	unsigned int string_total_len = 0;
	string_info *string_ptr = (string_info*)string_stack.get_base_addr();
	for(i=0; i<count; i++)
		string_total_len += kstrlen(string_ptr[i].get_name()) + 1;
	base = (char*)kmalloc(sizeof(unsigned int) + sizeof(string_info) * count + string_total_len);
	U_INT_VALUE(base) = string_total_len;
	base += sizeof(unsigned int);
	for(i=0; i<count; i++) {
		copy_len = sizeof(string_info);
		kmemcpy(base, &string_ptr[i], copy_len);
		base += copy_len;
	}
	for(i=0; i<count; i++) {
		copy_len = kstrlen(string_ptr[i].get_name()) + 1;
		kstrcpy(base, string_ptr[i].get_name());
		base += copy_len;
	}
	//%%%%%string rearrange end%%%%%%%%%%%%%%//
	count = this->varity_declare->global_varity_stack->get_count();
	varity_info *varity_ptr = (varity_info*)this->varity_declare->global_varity_stack->get_base_addr();
	name_total_len = 0;
	unsigned int bss_len = 0, data_len = 0;
	for(i=0; i<count; i++) {
		int align_byte, varity_size;
		name_total_len += kstrlen(varity_ptr[i].get_name()) + 1;
		align_byte = varity_ptr[i].get_element_size();
		varity_size = get_varity_size(0, varity_ptr->get_complex_ptr(), varity_ptr->get_complex_arg_count());
		if(memcheck(varity_ptr->get_content_ptr(), 0, varity_size)) {
			data_len = make_align(data_len, align_byte);
			data_len += varity_size;
		} else {
			bss_len = make_align(bss_len, align_byte);
			bss_len += varity_size;
		}
	}
	base = (char*)kmalloc(sizeof(unsigned int) + sizeof(varity_info) * count + data_len + bss_len + name_total_len);

	return ERROR_NO;
}

int c_interpreter::ulink(stack *stack_ptr, int mode)
{
	mid_code *mid_code_ptr = (mid_code*)stack_ptr->get_base_addr();
	unsigned int code_count = stack_ptr->get_count();
	int i, link_ret = 0;
	void *obj_ptr;
	char *symbol_name;
	for(i=0; i<code_count; i++) {
		void *opda_addr = NULL, *opdb_addr = NULL, *ret_addr = NULL;
		int opda_flag = 0, opdb_flag = 0, ret_flag = 0;
		if(mid_code_ptr[i].ret_operator >= CTL_CMD_NO)
			continue;
		if(mid_code_ptr[i].ret_operand_type == OPERAND_G_VARITY) {
			ret_flag = 1;
			if((obj_ptr = (void*)this->varity_declare->find((char*)mid_code_ptr[i].ret_addr)) != NULL)
				if(mode == LINK_ADDR)
					ret_addr = ((varity_info*)obj_ptr)->get_content_ptr();
				else if(mode == LINK_NUMBER)
					ret_addr = (void*)((varity_info*)obj_ptr - (varity_info*)this->varity_declare->global_varity_stack->get_base_addr());
				else
					ret_addr = (void*)((string_info*)name_stack.find((char*)mid_code_ptr[i].ret_addr) - (string_info*)name_stack.get_base_addr());
			else {
				symbol_name = (char*)mid_code_ptr[i].ret_addr;
				link_ret = ERROR_VARITY_NONEXIST;
				break;
			}
		} else if (mid_code_ptr[i].ret_operand_type == OPERAND_STRING) {
			if(mode == LINK_ADDR) {
				mid_code_ptr[i].ret_operand_type = OPERAND_G_VARITY;
				mid_code_ptr[i].ret_addr = (PLATFORM_WORD)((string_info*)string_stack.visit_element_by_index(mid_code_ptr[i].ret_addr))->get_name();
			}
		}
		if(mid_code_ptr[i].opda_operand_type == OPERAND_G_VARITY) {
			opda_flag = 1;
			if((obj_ptr = (void*)this->varity_declare->find((char*)mid_code_ptr[i].opda_addr)) != NULL)
				if(mode == LINK_ADDR)
					opda_addr = ((varity_info*)obj_ptr)->get_content_ptr();
				else if(mode == LINK_NUMBER)
					opda_addr = (void*)((varity_info*)obj_ptr - (varity_info*)this->varity_declare->global_varity_stack->get_base_addr());
				else
					opda_addr = (void*)((string_info*)name_stack.find((char*)mid_code_ptr[i].opda_addr) - (string_info*)name_stack.get_base_addr());
			else {
				symbol_name = (char*)mid_code_ptr[i].opda_addr;
				link_ret = ERROR_VARITY_NONEXIST;
				break;
			}
		} else if (mid_code_ptr[i].opda_operand_type == OPERAND_STRING) {
			if(mode == LINK_ADDR) {
				mid_code_ptr[i].opda_operand_type = OPERAND_G_VARITY;
				mid_code_ptr[i].opda_addr = (PLATFORM_WORD)((string_info*)string_stack.visit_element_by_index(mid_code_ptr[i].opda_addr))->get_name();
			}
		}
		if(mid_code_ptr[i].opdb_operand_type == OPERAND_G_VARITY) {
			opdb_flag = 1;
			if((obj_ptr = (void*)this->varity_declare->find((char*)mid_code_ptr[i].opdb_addr)) != NULL)
				if(mode == LINK_ADDR)
					opdb_addr = ((varity_info*)obj_ptr)->get_content_ptr();
				else if(mode == LINK_NUMBER)
					opdb_addr = (void*)((varity_info*)obj_ptr - (varity_info*)this->varity_declare->global_varity_stack->get_base_addr());
				else
					opdb_addr = (void*)((string_info*)name_stack.find((char*)mid_code_ptr[i].opdb_addr) - (string_info*)name_stack.get_base_addr());
			else {
				symbol_name = (char*)mid_code_ptr[i].opdb_addr;
				link_ret = ERROR_VARITY_NONEXIST;
				break;
			}
		} else if (mid_code_ptr[i].opdb_operand_type == OPERAND_STRING) {
			if(mode == LINK_ADDR) {
				mid_code_ptr[i].opdb_operand_type = OPERAND_G_VARITY;
				mid_code_ptr[i].opdb_addr = (PLATFORM_WORD)((string_info*)string_stack.visit_element_by_index(mid_code_ptr[i].opdb_addr))->get_name();
			}
		} else if (mid_code_ptr[i].opdb_operand_type == OPERAND_FUNCTION) {
			opdb_flag = 1;
			if((obj_ptr = (void*)this->function_declare->find((char*)mid_code_ptr[i].opdb_addr)) != NULL)
				if(mode == LINK_ADDR)
					opdb_addr = ((function_info*)obj_ptr);
				else if(mode == LINK_NUMBER)
					opdb_addr = (void*)((function_info*)obj_ptr - (function_info*)this->function_declare->function_stack_ptr->get_base_addr());
				else
					opdb_addr = (void*)((string_info*)name_stack.find((char*)mid_code_ptr[i].opdb_addr) - (string_info*)name_stack.get_base_addr());
			else {
				symbol_name = (char*)mid_code_ptr[i].opdb_addr;
				link_ret = ERROR_VARITY_NONEXIST;
				break;
			}
		}
		if(mid_code_ptr[i].ret_operator == OPT_CALL_FUNC) {
			opda_flag = 1;
			if((obj_ptr = (void*)this->function_declare->find((char*)mid_code_ptr[i].opda_addr)) != NULL)
				if(mode == LINK_ADDR)
					opda_addr = obj_ptr;
				else if(mode == LINK_NUMBER)
					opda_addr = (void*)((function_info*)obj_ptr - (function_info*)this->function_declare->function_stack_ptr->get_base_addr());
				else
					opda_addr = (void*)((string_info*)name_stack.find((char*)mid_code_ptr[i].opda_addr) - (string_info*)name_stack.get_base_addr());
			else {
				symbol_name = (char*)mid_code_ptr[i].opda_addr;
				link_ret = ERROR_NO_FUNCTION;
				break;
			}
		}
		/*if(!(opda_flag != 0 ^ opda_addr != NULL) && !(opdb_flag != 0 ^ opdb_addr != NULL) && !(ret_flag != 0 ^ ret_addr != NULL) && mid_code_ptr[i].ret_operator < CTL_CMD_NO) {
			if(ret_flag)
				mid_code_ptr[i].ret_addr = (long)ret_addr;
			if(opda_flag)
				mid_code_ptr[i].opda_addr = (long)opda_addr;
			if(opdb_flag)
				mid_code_ptr[i].opdb_addr = (long)opdb_addr;
		}*/
		if((opda_flag || opdb_flag || ret_flag) && mid_code_ptr[i].ret_operator < CTL_CMD_NO) {
			if(ret_flag)
				mid_code_ptr[i].ret_addr = (long)ret_addr;
			if(opda_flag)
				mid_code_ptr[i].opda_addr = (long)opda_addr;
			if(opdb_flag)
				mid_code_ptr[i].opdb_addr = (long)opdb_addr;
		}
	}
	switch(link_ret) {
	case ERROR_NO:
		return link_ret;
	case ERROR_VARITY_NONEXIST:
		error("varity %s not exist\n", symbol_name);
		break;
	case ERROR_NO_FUNCTION:
		error("function %s not exist\n", symbol_name);
		break;
	}
	return link_ret;
}

int c_interpreter::tlink(int mode)
{
	int entry_flag = 0;
	int count = this->function_declare->function_stack_ptr->get_count();
	function_info *function_base = (function_info*)this->function_declare->function_stack_ptr->get_base_addr();
	for(int i=0; i<count; i++) {
		if(!entry_flag && !kstrcmp(function_base[i].get_name(), "main")) {
			function_info tmp;
			kmemcpy(&tmp, &function_base[i], sizeof(function_info));
			kmemcpy(&function_base[i], &function_base[this->cstdlib_func_count], sizeof(function_info));
			kmemcpy(&function_base[this->cstdlib_func_count], &tmp, sizeof(function_info));
			entry_flag = 1;
		}
	}
	if(!entry_flag && mode == LINK_NUMBER) {
		error("no main function.\n");
		return ERROR_NOMAIN;
	}
	for(int i=0; i<count; i++) {
		if(!function_base[i].compile_func_flag) {
			int ret = this->ulink(&function_base[i].mid_code_stack, mode);
			if(ret) {
				error("function %s link error\n", function_base[i].get_name());
				return ret;
			}
		}
	}

	return ERROR_NO;
}

int c_interpreter::run_thread(function_info*)
{
	return ERROR_NO;
}

int c_interpreter::run_main(int stop, void *load_base, void *bss_base)
{
	int ret;
	function_info *function_ptr = (function_info*)this->function_declare->function_stack_ptr->get_base_addr() + this->cstdlib_func_count;
	stack *stack_ptr = &function_ptr->mid_code_stack;
	mid_code *pc = (mid_code*)stack_ptr->get_base_addr();
	if(stop)
		pc->break_flag |= BREAKPOINT_STEP;
	ret = this->exec_mid_code(pc, stack_ptr->get_count());
	vfree(load_base);
	vfree(bss_base);
	//TODO:清理所有导入符号
	return ret;
}

int c_interpreter::symbol_sr_status(int sr_direction)
{
	static int save_flag = SYMBOL_RESTORE;
	static int function_count = 0, varity_count = 0, struct_count = 0, string_count = 0, name_count = 0, varity_type_count = 0;
	if(sr_direction == SYMBOL_SAVE) {
		if(save_flag == SYMBOL_SAVE)
			return ERROR_SR_STATUS;
		save_flag = SYMBOL_SAVE;
		function_count = this->function_declare->function_stack_ptr->get_count();
		varity_count = this->varity_declare->global_varity_stack->get_count();
		struct_count = this->struct_declare->struct_stack_ptr->get_count();
		string_count = string_stack.get_count();
		name_count = name_stack.get_count();
		varity_type_count = varity_type_stack.count;
	} else if(sr_direction == SYMBOL_RESTORE) {
		int delta_count;
		int i;
		if(save_flag == SYMBOL_RESTORE)
			return ERROR_SR_STATUS;
		save_flag = SYMBOL_RESTORE;
		delta_count = this->function_declare->function_stack_ptr->get_count() - function_count;
		for(i=0; i<delta_count; i++)
			this->function_declare->function_stack_ptr->pop();
		delta_count = this->varity_declare->global_varity_stack->get_count() - varity_count;
		for(i=0; i<delta_count; i++)
			this->varity_declare->global_varity_stack->pop();
		delta_count = this->struct_declare->struct_stack_ptr->get_count();
		for(i=0; i<delta_count; i++)
			this->struct_declare->struct_stack_ptr->pop();
		string_stack.set_count(string_count);
		name_stack.set_count(name_count);
		varity_type_stack.count = varity_type_count;
	}
	return ERROR_NO;
}

int c_interpreter::load_ofile(char *file, int flag, void **load_base, void **bss_base_ret)
{
	void *file_ptr = kfopen(file, "rb");
	if(!file_ptr) {
		error("can't open file %s\n", file);
		return ERROR_FILE;
	}
	unsigned int function_total_size;
	int i, j;
	kfread(&this->compile_info, sizeof(compile_info_t), 1, file_ptr);
	if(compile_info.sum32 != calc_sum32((int*)&compile_info, sizeof(this->compile_info) - sizeof(int))) {
		error("Not an ELF file.\n");
		return ERROR_FILE;
	}
	function_info *function_info_ptr = (function_info*)dmalloc(sizeof(function_info), "all infos base");
	kfread(&this->compile_string_info, sizeof(compile_string_info_t), 1, file_ptr);
	void *str_base = dmalloc(compile_string_info.string_size + compile_string_info.name_size, "string base");
	//string_info *string_info_ptr = (string_info*)str_base;
	kfread((char*)str_base, 1, compile_string_info.string_size + compile_string_info.name_size, file_ptr);
	char *str_data_base = (char*)str_base;
	unsigned short *str_map_table = (unsigned short*)dmalloc(sizeof(short) * compile_string_info.string_count, "string stack map");
	unsigned short *name_map_table = (unsigned short*)dmalloc(sizeof(short) * compile_string_info.name_count, "name stack map");
	for(i=0; i<compile_string_info.string_count; i++) {
		void *strptr = string_stack.find(str_data_base);
		if(strptr)
			str_map_table[i] = (string_info*)strptr - (string_info*)string_stack.get_base_addr();
		else {//TODO:string not in name fifo.
			string_info tmp;
			tmp.set_name(name_fifo.write(str_data_base));
			str_map_table[i] = string_stack.get_count();
			string_stack.push(&tmp);
		}
		str_data_base = (char*)str_data_base + kstrlen((char*)str_data_base) + 1;
	}
	if(compile_info.extra_flag || compile_info.export_flag == EXPORT_FLAG_LINK) {
		for(i=0; i<compile_string_info.name_count; i++) {
			void *strptr = name_stack.find(str_data_base);
			if(strptr)
				name_map_table[i] = (string_info*)strptr - (string_info*)name_stack.get_base_addr();
			else {
				string_info tmp;
				tmp.set_name(name_fifo.write(str_data_base));
				name_map_table[i] = name_stack.get_count();
				name_stack.push(&tmp);
			}
			str_data_base = (char*)str_data_base + kstrlen((char*)str_data_base) + 1;
		}
	}
	vfree(str_base);
	void *base = dmalloc(this->compile_info.total_size, "load file space");
	*load_base = base;
	kfread(&this->compile_function_info, sizeof(compile_function_info_t), 1, file_ptr);
	mid_code *mid_code_ptr = (mid_code*)base;
	stack *arg_varity_ptr = (stack*)((char*)mid_code_ptr + this->compile_function_info.mid_code_size);
	varity_info *local_varity_ptr = (varity_info*)((char*)arg_varity_ptr + this->compile_function_info.arg_size);
	char *source_code_ptr = (char*)local_varity_ptr + this->compile_function_info.local_varity_size;
	mid_code **row_code_ptr = (mid_code**)((char*)source_code_ptr + make_align(this->compile_function_info.source_code_size, PLATFORM_WORD_LEN));
	kfread(mid_code_ptr, 1, this->compile_function_info.mid_code_size, file_ptr);
	kfread(arg_varity_ptr, 1, this->compile_function_info.arg_size, file_ptr);
	kfread(local_varity_ptr, 1, this->compile_function_info.local_varity_size, file_ptr);
	kfread(source_code_ptr, 1, this->compile_function_info.source_code_size, file_ptr);
	kfread(row_code_ptr, 1, this->compile_function_info.code_map_size, file_ptr);
	unsigned int function_begin_count = this->function_declare->function_stack_ptr->get_count();
	unsigned int struct_begin_count = this->struct_declare->struct_stack_ptr->get_count();
	unsigned int varity_begin_count = this->varity_declare->global_varity_stack->get_count();
	for(i=0; i<compile_function_info.function_count; i++) {
		kfread(function_info_ptr, sizeof(function_info), 1, file_ptr);
		kmemcpy(&function_info_ptr->local_varity_stack, &c_interpreter::language_elment_space.l_varity_list, sizeof(void*));//copy virtual table
		function_info_ptr->mid_code_stack.set_base(mid_code_ptr);
		if(compile_info.extra_flag) {
			function_info_ptr->arg_list = (stack*)arg_varity_ptr++;
			function_info_ptr->arg_list->set_base(arg_varity_ptr);
			function_info_ptr->local_varity_stack.set_base(local_varity_ptr);
			function_info_ptr->buffer = source_code_ptr;
			function_info_ptr->row_code_ptr = row_code_ptr;
			function_info_ptr->row_begin_pos = (unsigned int*)dmalloc(sizeof(char*) * function_info_ptr->row_line, "");
			for(j=0; j<function_info_ptr->row_line; j++) {
				function_info_ptr->row_code_ptr[j] = mid_code_ptr + (int)function_info_ptr->row_code_ptr[j];
				if(j == 0)
					function_info_ptr->row_begin_pos[j] = 0;
				else
					function_info_ptr->row_begin_pos[j] = function_info_ptr->row_begin_pos[j - 1] + kstrlen(source_code_ptr + function_info_ptr->row_begin_pos[j - 1]) + 1;
			}
			source_code_ptr += function_info_ptr->wptr;
			row_code_ptr += function_info_ptr->row_line;
			arg_varity_ptr = (stack*)((varity_info*)arg_varity_ptr + function_info_ptr->arg_list->get_count());
			local_varity_ptr += function_info_ptr->local_varity_stack.get_count();
		}
		if(compile_info.export_flag == EXPORT_FLAG_LINK || compile_info.extra_flag)
			function_info_ptr->set_name(((string_info*)name_stack.visit_element_by_index(name_map_table[(int)function_info_ptr ->get_name()]))->get_name());
		else
			function_info_ptr->set_name(NULL);
		mid_code_ptr += function_info_ptr->mid_code_stack.get_count();
		this->function_declare->function_stack_ptr->push(function_info_ptr);
	}
	function_total_size = this->compile_function_info.mid_code_size + make_align(this->compile_function_info.source_code_size, PLATFORM_WORD_LEN) + this->compile_function_info.arg_size + this->compile_function_info.code_map_size;
	base = (char*)base + make_align(function_total_size, 4);
	struct_info *struct_info_ptr = (struct_info*)function_info_ptr;
	unsigned short *struct_map_table;
	if(compile_info.extra_flag) {
		struct_info *struct_dup_ptr;
		kfread(&this->compile_struct_info, sizeof(compile_struct_info_t), 1, file_ptr);
		struct_map_table = (unsigned short*)dmalloc(sizeof(short) * compile_struct_info.struct_count, "struct map");
		arg_varity_ptr = (stack*)base;
		kfread(arg_varity_ptr, compile_struct_info.varity_size, 1, file_ptr);
		for(i=0; i<compile_struct_info.struct_count; i++) {
			kfread(struct_info_ptr, sizeof(struct_info), 1, file_ptr);
			struct_info_ptr->varity_stack_ptr = arg_varity_ptr++;
			struct_info_ptr->varity_stack_ptr->set_base(arg_varity_ptr);
			//struct_info_ptr->type_info_ptr[1] = (uint)struct_info_ptr;
			struct_info_ptr->set_name(((string_info*)name_stack.visit_element_by_index(name_map_table[(int)struct_info_ptr ->get_name()]))->get_name());
			struct_dup_ptr = this->struct_declare->find(struct_info_ptr->get_name());
			if(struct_dup_ptr) {//TODO:struct重复定义时检查结构，定义不一致时直接报错
				struct_map_table[i] = struct_dup_ptr - (struct_info*)this->struct_declare->struct_stack_ptr->get_base_addr();
			} else {
				kmemcpy(struct_info_ptr->varity_stack_ptr, &this->interprete_need_ptr->mid_code_stack, sizeof(void*));//copy virtual table
				struct_map_table[i] = this->struct_declare->struct_stack_ptr->get_count();
				this->struct_declare->struct_stack_ptr->push(struct_info_ptr);
			}
			arg_varity_ptr = (stack*)((varity_info*)arg_varity_ptr + struct_info_ptr->varity_stack_ptr->get_count());
		}
	}
	base = (char*)base + compile_struct_info.varity_size;
	compile_varity_info_t compile_varity_info;
	kfread(&compile_varity_info, sizeof(compile_varity_info_t), 1, file_ptr);
	varity_type_stack_t *varity_type_stack_ptr = (varity_type_stack_t*)function_info_ptr;
	varity_type_stack_t *varity_type_info_ptr;
	PLATFORM_WORD *varity_type_ptr;
	if(compile_info.extra_flag) {
		kfread(varity_type_stack_ptr, sizeof(varity_type_stack), 1, file_ptr);
		varity_type_info_ptr = (varity_type_stack_t*)dmalloc(sizeof(varity_type_stack_t) + make_align(varity_type_stack_ptr->count, PLATFORM_WORD_LEN) + varity_type_stack_ptr->count * sizeof(void*) + compile_varity_info.type_size + sizeof(varity_info), "varity type info");
		varity_type_ptr = (PLATFORM_WORD*)((char*)varity_type_info_ptr + sizeof(varity_type_stack_t) + make_align(varity_type_stack_ptr->count, PLATFORM_WORD_LEN) + varity_type_stack_ptr->count * sizeof(void*));
	} else {
		varity_type_info_ptr = (varity_type_stack_t*)dmalloc(sizeof(varity_info), "varity type info");
		varity_type_ptr = (PLATFORM_WORD*)varity_type_info_ptr;
	}
	if(compile_info.extra_flag) {
		varity_type_stack_ptr->arg_count = (char*)(varity_type_info_ptr + 1);
		varity_type_stack_ptr->type_info_addr = (PLATFORM_WORD**)(varity_type_stack_ptr->arg_count + make_align(varity_type_stack_ptr->count, PLATFORM_WORD_LEN));
		kfread(varity_type_stack_ptr->arg_count, make_align(varity_type_stack_ptr->count, PLATFORM_WORD_LEN), 1, file_ptr);
		kfread(varity_type_ptr, 1, compile_varity_info.type_size - make_align(varity_type_stack_ptr->count, PLATFORM_WORD_LEN), file_ptr);
		varity_type_stack_ptr->type_info_addr[0] = varity_type_ptr;
		for(i=1; i<varity_type_stack_ptr->count; i++) {
			varity_type_stack_ptr->type_info_addr[i] = (PLATFORM_WORD*)((char*)varity_type_stack_ptr->type_info_addr[i - 1] + (varity_type_stack_ptr->arg_count[i - 1] + 1) * PLATFORM_WORD_LEN);
			for(j=varity_type_stack_ptr->arg_count[i]; j>=1; j--) {
				if(GET_COMPLEX_TYPE(varity_type_stack_ptr->type_info_addr[i][j]) == COMPLEX_BASIC && GET_COMPLEX_DATA(varity_type_stack_ptr->type_info_addr[i][j]) == STRUCT) {
					if(i == STRUCT) continue;//type_info[STRUCT][1] = 0,没法重做map
					varity_type_stack_ptr->type_info_addr[i][j - 1] = (PLATFORM_WORD)this->struct_declare->struct_stack_ptr->visit_element_by_index(struct_map_table[varity_type_stack_ptr->type_info_addr[i][j - 1]]);
					((struct_info*)varity_type_stack_ptr->type_info_addr[i][j - 1])->type_info_ptr = varity_type_stack_ptr->type_info_addr[i];
					((struct_info*)varity_type_stack_ptr->type_info_addr[i][j - 1])->type_info_ptr[1] = varity_type_stack_ptr->type_info_addr[i][j - 1];
					j--;
				}
			}
		}
		kmemcpy(varity_type_info_ptr, varity_type_stack_ptr, sizeof(varity_type_stack_t));
	}
	varity_info *varity_info_ptr = (varity_info*)((char*)varity_type_ptr + compile_varity_info.type_size);
	for(i=0; i<compile_varity_info.varity_count; i++) {
		kfread(varity_info_ptr, sizeof(varity_info), 1, file_ptr);
		if(compile_info.extra_flag || compile_info.export_flag == EXPORT_FLAG_LINK)
			varity_info_ptr->set_name(((string_info*)name_stack.visit_element_by_index(name_map_table[(int)varity_info_ptr->get_name()]))->get_name());
		if(compile_info.extra_flag) {
			int type_no = varity_type_stack.find(varity_info_ptr->get_complex_arg_count(), (varity_type_stack_ptr->type_info_addr[(int)varity_info_ptr->get_complex_ptr()]));
			if(type_no >= 0) {
				varity_info_ptr->config_complex_info(varity_info_ptr->get_complex_arg_count(), (PLATFORM_WORD*)varity_type_stack.type_info_addr[type_no]);
			} else {
				int complex_size = (varity_info_ptr->get_complex_arg_count() + 1) * sizeof(void*);
				PLATFORM_WORD *complex_ptr = (PLATFORM_WORD*)dmalloc(complex_size, "complex info ptr");
				kmemcpy(complex_ptr, varity_type_stack_ptr->type_info_addr[(int)varity_info_ptr->get_complex_ptr()], complex_size);
				varity_info_ptr->config_complex_info(varity_info_ptr->get_complex_arg_count(), complex_ptr);
				varity_type_stack.arg_count[varity_type_stack.count] = varity_info_ptr->get_complex_arg_count();
				varity_type_stack.type_info_addr[varity_type_stack.count] = complex_ptr;
				varity_type_stack.push();
			}
		}
		this->varity_declare->global_varity_stack->push(varity_info_ptr);
	}

	for(i=0; i<compile_function_info.function_count; i++) {
		function_info *function_info_ptr = (function_info*)this->function_declare->function_stack_ptr->get_base_addr();
		if(compile_info.extra_flag) {
			varity_info_ptr = (varity_info*)function_info_ptr[i + function_begin_count].arg_list->get_base_addr();
			for(j=0; j<function_info_ptr[i + function_begin_count].arg_list->get_count(); j++) {
				int type_no = varity_type_stack.find(varity_info_ptr[j].get_complex_arg_count(), varity_type_stack_ptr->type_info_addr[(int)varity_info_ptr[j].get_complex_ptr()]);
				if(type_no >= 0) {
					varity_info_ptr[j].config_complex_info(varity_info_ptr[j].get_complex_arg_count(), (PLATFORM_WORD*)varity_type_stack.type_info_addr[type_no]);
				} else {
					int complex_size = (varity_info_ptr[j].get_complex_arg_count() + 1) * sizeof(void*);
					PLATFORM_WORD *complex_ptr = (PLATFORM_WORD*)dmalloc(complex_size, "complex info ptr");
					kmemcpy(complex_ptr, varity_type_stack_ptr->type_info_addr[(int)varity_info_ptr[j].get_complex_ptr()], complex_size);
					varity_info_ptr[j].config_complex_info(varity_info_ptr[j].get_complex_arg_count(), complex_ptr);
					varity_type_stack.arg_count[varity_type_stack.count] = varity_info_ptr[j].get_complex_arg_count();
					varity_type_stack.type_info_addr[varity_type_stack.count] = complex_ptr;
					varity_type_stack.push();
				}
			}
			varity_info_ptr = (varity_info*)function_info_ptr[i + function_begin_count].local_varity_stack.get_base_addr();
			for(j=0; j<function_info_ptr[i + function_begin_count].local_varity_stack.get_count(); j++) {
				varity_info_ptr[j].set_name(((string_info*)name_stack.visit_element_by_index(name_map_table[(int)varity_info_ptr[j].get_name()]))->get_name());
				int type_no = varity_type_stack.find(varity_info_ptr[j].get_complex_arg_count(), varity_type_stack_ptr->type_info_addr[(int)varity_info_ptr[j].get_complex_ptr()]);
				if(type_no >= 0) {
					varity_info_ptr[j].config_complex_info(varity_info_ptr[j].get_complex_arg_count(), (PLATFORM_WORD*)varity_type_stack.type_info_addr[type_no]);
				} else {
					int complex_size = (varity_info_ptr[j].get_complex_arg_count() + 1) * sizeof(void*);
					PLATFORM_WORD *complex_ptr = (PLATFORM_WORD*)dmalloc(complex_size, "complex info ptr");
					kmemcpy(complex_ptr, varity_type_stack_ptr->type_info_addr[(int)varity_info_ptr[j].get_complex_ptr()], complex_size);
					varity_info_ptr[j].config_complex_info(varity_info_ptr[j].get_complex_arg_count(), complex_ptr);
					varity_type_stack.arg_count[varity_type_stack.count] = varity_info_ptr[j].get_complex_arg_count();
					varity_type_stack.type_info_addr[varity_type_stack.count] = complex_ptr;
					varity_type_stack.push();
				}
			}
		}
	}
	if(compile_info.extra_flag) {
		for(i=0; i<compile_struct_info.struct_count; i++) {
			struct_info_ptr = (struct_info*)this->struct_declare->struct_stack_ptr->get_base_addr() + struct_begin_count;
			varity_info_ptr = (varity_info*)struct_info_ptr[i].varity_stack_ptr->get_base_addr();
			for(j=0; j<struct_info_ptr[i].varity_stack_ptr->get_count(); j++) {
				int type_no = varity_type_stack.find(varity_info_ptr[j].get_complex_arg_count(), varity_type_stack_ptr->type_info_addr[(int)varity_info_ptr[j].get_complex_ptr()]);
				if(type_no >= 0) {
					varity_info_ptr[j].config_complex_info(varity_info_ptr[j].get_complex_arg_count(), (PLATFORM_WORD*)varity_type_stack.type_info_addr[type_no]);
				} else {
					int complex_size = (varity_info_ptr[j].get_complex_arg_count() + 1) * sizeof(void*);
					PLATFORM_WORD *complex_ptr = (PLATFORM_WORD*)dmalloc(complex_size, "complex info ptr");
					kmemcpy(complex_ptr, varity_type_stack_ptr->type_info_addr[(int)varity_info_ptr[j].get_complex_ptr()], complex_size);
					varity_info_ptr[j].config_complex_info(varity_info_ptr[j].get_complex_arg_count(), complex_ptr);
					varity_type_stack.arg_count[varity_type_stack.count] = varity_info_ptr[j].get_complex_arg_count();
					varity_type_stack.type_info_addr[varity_type_stack.count] = complex_ptr;
					varity_type_stack.push();
				}
			}
		}
	}
	vfree(varity_type_info_ptr);
	base = (void*)make_align((long)base, 4);
	kfread((char*)base, 1, compile_varity_info.data_size, file_ptr);
	void *bss_base = dmalloc(compile_varity_info.bss_size, "bss space");
	*bss_base_ret = bss_base;
	varity_info_ptr = (varity_info*)this->varity_declare->global_varity_stack->visit_element_by_index(varity_begin_count);
	for(i=0; i<compile_varity_info.init_varity_count; i++)
		varity_info_ptr[i].set_content_ptr((char*)base + (int)varity_info_ptr[i].get_content_ptr());
	varity_info_ptr = &varity_info_ptr[i];
	for(i=0; i<compile_varity_info.varity_count - compile_varity_info.init_varity_count; i++)
		varity_info_ptr[i].set_content_ptr((char*)bss_base + (int)varity_info_ptr[i].get_content_ptr());
	if(compile_info.export_flag == EXPORT_FLAG_EXEC) {
		varity_info *varity_base = (varity_info*)this->varity_declare->global_varity_stack->get_base_addr() + varity_begin_count;
		function_info *function_base = (function_info*)this->function_declare->function_stack_ptr->get_base_addr() + function_begin_count;
		for(int j=0; j<compile_function_info.function_count; j++) {
			unsigned int code_count = function_base[j].mid_code_stack.get_count();
			mid_code_ptr = (mid_code*)function_base[j].mid_code_stack.get_base_addr();
			for(i=0; i<code_count; i++) {
				if(mid_code_ptr[i].ret_operator >= CTL_CMD_NO)
					continue;
				if(mid_code_ptr[i].ret_operand_type == OPERAND_G_VARITY) {
					mid_code_ptr[i].ret_addr = (PLATFORM_WORD)varity_base[mid_code_ptr[i].ret_addr].get_content_ptr();
				} else if (mid_code_ptr[i].ret_operand_type == OPERAND_STRING) {
					//mid_code_ptr[i].ret_operand_type = OPERAND_G_VARITY;
				}
				if(mid_code_ptr[i].opda_operand_type == OPERAND_G_VARITY) {
					mid_code_ptr[i].opda_addr = (PLATFORM_WORD)varity_base[mid_code_ptr[i].opda_addr].get_content_ptr();
				} else if (mid_code_ptr[i].opda_operand_type == OPERAND_STRING) {
					mid_code_ptr[i].opda_operand_type = OPERAND_G_VARITY;
					mid_code_ptr[i].opda_addr = (long)((string_info*)string_stack.visit_element_by_index(str_map_table[mid_code_ptr[i].opda_addr]))->get_name();
				}
				if(mid_code_ptr[i].opdb_operand_type == OPERAND_G_VARITY) {
					mid_code_ptr[i].opdb_addr = (PLATFORM_WORD)varity_base[mid_code_ptr[i].opdb_addr].get_content_ptr();
				} else if (mid_code_ptr[i].opdb_operand_type == OPERAND_FUNCTION) {
					mid_code_ptr[i].opdb_addr = (PLATFORM_WORD)&function_base[mid_code_ptr[i].opdb_addr - this->cstdlib_func_count];
				} else if (mid_code_ptr[i].opdb_operand_type == OPERAND_STRING) {
					mid_code_ptr[i].opdb_operand_type = OPERAND_G_VARITY;
					mid_code_ptr[i].opdb_addr = (long)((string_info*)string_stack.visit_element_by_index(str_map_table[mid_code_ptr[i].opdb_addr]))->get_name();
				}
				if(mid_code_ptr[i].ret_operator == OPT_CALL_FUNC) {
					mid_code_ptr[i].opda_addr = (PLATFORM_WORD)&function_base[mid_code_ptr[i].opda_addr - this->cstdlib_func_count];
				}
			}
		}
	} else if(compile_info.export_flag == EXPORT_FLAG_LINK) {
		function_info *function_base = (function_info*)this->function_declare->function_stack_ptr->get_base_addr() + function_begin_count;
		for(int j=0; j<compile_function_info.function_count; j++) {
			unsigned int code_count = function_base[j].mid_code_stack.get_count();
			mid_code_ptr = (mid_code*)function_base[j].mid_code_stack.get_base_addr();
			for(i=0; i<code_count; i++) {
				if(mid_code_ptr[i].ret_operator >= CTL_CMD_NO)
					continue;
				if(mid_code_ptr[i].ret_operand_type == OPERAND_G_VARITY) {
					mid_code_ptr[i].ret_addr = (long)((string_info*)name_stack.visit_element_by_index(name_map_table[mid_code_ptr[i].ret_addr]))->get_name();
				} else if (mid_code_ptr[i].ret_operand_type == OPERAND_STRING) {
					//mid_code_ptr[i].ret_operand_type = OPERAND_G_VARITY;
				}
				if(mid_code_ptr[i].opda_operand_type == OPERAND_G_VARITY) {
					mid_code_ptr[i].opda_addr = (long)((string_info*)name_stack.visit_element_by_index(name_map_table[mid_code_ptr[i].opda_addr]))->get_name();
				} else if (mid_code_ptr[i].opda_operand_type == OPERAND_STRING) {
					//mid_code_ptr[i].opda_operand_type = OPERAND_G_VARITY;
				}
				if(mid_code_ptr[i].opdb_operand_type == OPERAND_G_VARITY) {
					mid_code_ptr[i].opdb_addr = (long)((string_info*)name_stack.visit_element_by_index(name_map_table[mid_code_ptr[i].opdb_addr]))->get_name();
				} else if (mid_code_ptr[i].opdb_operand_type == OPERAND_FUNCTION) {
					mid_code_ptr[i].opdb_addr = (long)((string_info*)name_stack.visit_element_by_index(name_map_table[mid_code_ptr[i].opdb_addr]))->get_name();
				} else if (mid_code_ptr[i].opdb_operand_type == OPERAND_STRING) {
					//mid_code_ptr[i].opdb_operand_type = OPERAND_G_VARITY;
				}
				if(mid_code_ptr[i].ret_operator == OPT_CALL_FUNC) {
					mid_code_ptr[i].opda_addr = (long)((string_info*)name_stack.visit_element_by_index(name_map_table[mid_code_ptr[i].opda_addr]))->get_name();
				}
			}
		}
	}
	vfree(function_info_ptr);
	vfree(str_map_table);
	vfree(name_map_table);
	kfclose(file_ptr);
	return ERROR_NO;
}

int c_interpreter::write_ofile(const char *file, int export_flag, int extra_flag)
{
	int count, i, j;
	void *file_ptr = kfopen(file, "wb");
	kmemset(&this->compile_info, 0, sizeof(compile_info_t));
	kmemset(&this->compile_function_info, 0, sizeof(compile_function_info_t));
	kmemset(&this->compile_string_info, 0, sizeof(compile_string_info_t));
	kmemset(&this->compile_varity_info, 0, sizeof(compile_varity_info));
	kmemset(&this->compile_struct_info, 0, sizeof(compile_struct_info));
	this->compile_info.export_flag = export_flag;//TODO:use macro
	this->compile_info.extra_flag = extra_flag;
	debug("export flag=%d, extra_flag=%d\n", export_flag, extra_flag);
	count = this->function_declare->function_stack_ptr->get_count();
	debug("function count=%d\n", count);
	function_info *function_ptr = (function_info*)this->function_declare->function_stack_ptr->get_base_addr();
	for(this->compile_function_info.function_count=0,i=0; i<count; i++) {
		if(function_ptr[i].compile_func_flag == 0) {
			if(this->compile_info.extra_flag >= EXTRA_FLAG_DEBUG) {
				this->compile_function_info.source_code_size += function_ptr[i].wptr;
				this->compile_function_info.arg_size += function_ptr[i].arg_list->get_count() * sizeof(varity_info) + sizeof(stack);
				this->compile_function_info.local_varity_size += function_ptr[i].local_varity_stack.get_count() * sizeof(varity_info);
				this->compile_function_info.code_map_size += function_ptr[i].row_line * sizeof(mid_code*);
			}
			this->compile_function_info.mid_code_size += function_ptr[i].mid_code_stack.get_count() * sizeof(mid_code);
			this->compile_function_info.function_count++;
		}
	}
	debug("source=%d,arg=%d,lvariable=%d,code map=%d\n", this->compile_function_info.source_code_size, this->compile_function_info.arg_size, this->compile_function_info.local_varity_size, this->compile_function_info.code_map_size);
	count = string_stack.get_count();
	this->compile_string_info.string_count = count;
	string_info *string_info_ptr = (string_info*)string_stack.get_base_addr();
	for(i=0; i<count; i++)
		this->compile_string_info.string_size += kstrlen(string_info_ptr[i].get_name()) + 1;
	debug("str count=%d, str size=%d\n", count, this->compile_string_info.string_size);
	if(compile_info.extra_flag || compile_info.export_flag == EXPORT_FLAG_LINK) {
		this->compile_string_info.name_count = name_stack.get_count();
		string_info_ptr = (string_info*)name_stack.get_base_addr();
		for(i=0; i<this->compile_string_info.name_count; i++)
			this->compile_string_info.name_size += kstrlen(string_info_ptr[i].get_name()) + 1;
	}
	debug("name count=%d, name size=%d\n", this->compile_string_info.name_count, this->compile_string_info.name_size);
	struct_info *struct_ptr = (struct_info*)this->struct_declare->struct_stack_ptr->get_base_addr();
	if(compile_info.extra_flag) {
		this->compile_struct_info.struct_count = this->struct_declare->struct_stack_ptr->get_count();
		for(i=0; i<this->compile_struct_info.struct_count; i++) {
			this->compile_struct_info.varity_size += struct_ptr[i].varity_stack_ptr->get_count() * sizeof(varity_info) + sizeof(stack);
		}
	}
	debug("struct count=%d, struct size=%d\n", this->compile_struct_info.struct_count, this->compile_struct_info.varity_size);
	count = this->varity_declare->global_varity_stack->get_count();
	varity_info *varity_info_ptr = (varity_info*)this->varity_declare->global_varity_stack->get_base_addr();
	unsigned int varity_size, varity_align_size;
	for(i=0, j=count-1;;) {
		while(varity_info_ptr[i].get_content_ptr()) {
			if(varity_info_ptr[i].get_complex_arg_count()) {
				varity_size = get_varity_size(0, varity_info_ptr[i].get_complex_ptr(), varity_info_ptr[i].get_complex_arg_count());
				varity_align_size = get_align_size(varity_info_ptr[i].get_complex_arg_count(), varity_info_ptr[i].get_complex_ptr());
			} else {
				varity_size = varity_info_ptr[i].get_size();//TODO:两个分支都用get_size
				varity_align_size = (unsigned int)varity_info_ptr[i].get_complex_ptr();
			}
			this->compile_varity_info.varity_count++;
			if(kmemchk(varity_info_ptr[i].get_content_ptr(), 0, varity_size)) {
				this->compile_varity_info.init_varity_count++;
				this->compile_varity_info.data_size = make_align(compile_varity_info.data_size, varity_align_size) + varity_size;
			} else {
				this->compile_varity_info.bss_size = make_align(compile_varity_info.bss_size, varity_align_size) + varity_size;
			}
			if(++i >= j)
				break;
		}
		while(!varity_info_ptr[j].get_content_ptr()) {
			if(i >= --j)
				break;
		}
		if(i < j) {
			varity_info tmp;
			kmemcpy(&tmp, varity_info_ptr + i, sizeof(varity_info));
			kmemcpy(varity_info_ptr + i, varity_info_ptr + j, sizeof(varity_info));
			kmemcpy(varity_info_ptr + j, &tmp, sizeof(varity_info));
		} else if(i > j)
			break;
	}
	for(i=0, j=compile_varity_info.varity_count-1;;) {
		for(;i<compile_varity_info.varity_count;) {
			//varity_size = get_varity_size(0, varity_info_ptr[i].get_complex_ptr(), varity_info_ptr[i].get_complex_arg_count());
			varity_size = varity_info_ptr[i].get_size();
			if(kmemchk(varity_info_ptr[i].get_content_ptr(), 0, varity_size))
				i++;
			else
				break;
		}
		for(;j>=0;) {
			//varity_size = get_varity_size(0, varity_info_ptr[j].get_complex_ptr(), varity_info_ptr[j].get_complex_arg_count());
			varity_size = varity_info_ptr[j].get_size();
			if(!kmemchk(varity_info_ptr[j].get_content_ptr(), 0, varity_size))
				j--;
			else
				break;
		}
		if(i < j) {
			varity_info tmp;
			kmemcpy(&tmp, varity_info_ptr + i, sizeof(varity_info));
			kmemcpy(varity_info_ptr + i, varity_info_ptr + j, sizeof(varity_info));
			kmemcpy(varity_info_ptr + j, &tmp, sizeof(varity_info));
		} else if(i > j)
			break;
	}
	debug("variable count=%d, data size=%d, bss size=%d\n", count, this->compile_varity_info.data_size, this->compile_varity_info.bss_size);
	this->compile_varity_info.type_size = 0;
	if(this->compile_info.extra_flag >= EXTRA_FLAG_DEBUG) {
		compile_varity_info.type_size = make_align(varity_type_stack.count, PLATFORM_WORD_LEN);
		for(i=0; i<varity_type_stack.count; i++)
			compile_varity_info.type_size += (varity_type_stack.arg_count[i] + 1) * PLATFORM_WORD_LEN;
	}
	debug("type count=%d,size=%d\n", varity_type_stack.count, compile_varity_info.type_size);
	this->compile_info.total_size = make_align(compile_function_info.source_code_size, PLATFORM_WORD_LEN)
								+ compile_function_info.mid_code_size + compile_function_info.arg_size
								+ compile_function_info.code_map_size + compile_function_info.local_varity_size
								+ compile_varity_info.data_size + compile_struct_info.varity_size;
	debug("total size=%d\n", this->compile_info.total_size);
	this->compile_info.sum32 = calc_sum32((int*)&this->compile_info, sizeof(this->compile_info) - sizeof(int));
	kfwrite(&this->compile_info, sizeof(compile_info), 1, file_ptr);
	kfwrite(&this->compile_string_info, sizeof(compile_string_info_t), 1, file_ptr);
	//kfwrite(string_info_ptr, sizeof(string_info), this->compile_string_info.string_count, file_ptr);
	string_info_ptr = (string_info*)string_stack.get_base_addr();
	for(i=0; i<this->compile_string_info.string_count; i++)
		kfwrite(string_info_ptr[i].get_name(), 1, kstrlen(string_info_ptr[i].get_name()) + 1, file_ptr);
	if(compile_info.extra_flag || compile_info.export_flag == EXPORT_FLAG_LINK) {
		string_info_ptr = (string_info*)name_stack.get_base_addr();
		for(i=0; i<this->compile_string_info.name_count; i++)
			kfwrite(string_info_ptr[i].get_name(), 1, kstrlen(string_info_ptr[i].get_name()) + 1, file_ptr);
	}
	kfwrite(&this->compile_function_info, sizeof(compile_function_info), 1, file_ptr);
	count = this->function_declare->function_stack_ptr->get_count();
	for(i=0; i<count; i++)
		if(!function_ptr[i].compile_func_flag)
			kfwrite(function_ptr[i].mid_code_stack.get_base_addr(), sizeof(mid_code), function_ptr[i].mid_code_stack.get_count(), file_ptr);
	if(this->compile_info.extra_flag >= EXTRA_FLAG_DEBUG) {
		for(i=0; i<count; i++) 
			if(!function_ptr[i].compile_func_flag) {
				unsigned int arg_count = function_ptr[i].arg_list->get_count();
				kfwrite(function_ptr[i].arg_list, sizeof(stack), 1, file_ptr);
				for(int j=0; j<arg_count; j++) {
					varity_info* arg_varity_ptr = (varity_info*)function_ptr[i].arg_list->visit_element_by_index(j);
					PLATFORM_WORD *complex_ptr = arg_varity_ptr->get_complex_ptr();
					unsigned type_no = varity_type_stack.find(arg_varity_ptr->get_complex_arg_count(), complex_ptr);
					arg_varity_ptr->config_complex_info(arg_varity_ptr->get_complex_arg_count(), (PLATFORM_WORD*)type_no);
					kfwrite(arg_varity_ptr, sizeof(varity_info), 1, file_ptr);
					arg_varity_ptr->config_complex_info(arg_varity_ptr->get_complex_arg_count(), complex_ptr);
				}
			}
		for(i=0; i<count; i++) 
			if(!function_ptr[i].compile_func_flag) {
				unsigned int arg_count = function_ptr[i].local_varity_stack.get_count();
				for(int j=0; j<arg_count; j++) {
					varity_info* arg_varity_ptr = (varity_info*)function_ptr[i].local_varity_stack.visit_element_by_index(j);
					int name_no = (string_info*)name_stack.find(arg_varity_ptr->get_name()) - (string_info*)name_stack.get_base_addr();
					arg_varity_ptr->set_name((char*)name_no);
					PLATFORM_WORD *complex_ptr = arg_varity_ptr->get_complex_ptr();
					unsigned int type_no = varity_type_stack.find(arg_varity_ptr->get_complex_arg_count(), complex_ptr);
					arg_varity_ptr->config_complex_info(arg_varity_ptr->get_complex_arg_count(), (PLATFORM_WORD*)type_no);
					kfwrite(arg_varity_ptr, sizeof(varity_info), 1, file_ptr);
					arg_varity_ptr->config_complex_info(arg_varity_ptr->get_complex_arg_count(), complex_ptr);
				}
			}
		for(i=0; i<count; i++)
			if(!function_ptr[i].compile_func_flag) {
				if(function_ptr[i].buffer)
					kfwrite(function_ptr[i].buffer, 1, function_ptr[i].wptr, file_ptr);
			}
		for(i=0; i<count; i++)
			if(!function_ptr[i].compile_func_flag) {
				if(function_ptr[i].row_code_ptr)
					for(j=0; j<function_ptr[i].row_line; j++) {
						int offset = function_ptr[i].row_code_ptr[j] - (mid_code*)function_ptr[i].mid_code_stack.get_base_addr();
						kfwrite(&offset, sizeof(offset), 1, file_ptr);
					}
			}
	}
	for(i=0; i<count; i++)
		if(!function_ptr[i].compile_func_flag) {
			if(this->compile_info.export_flag == EXPORT_FLAG_LINK || compile_info.extra_flag)
				function_ptr[i].set_name((char*)((string_info*)name_stack.find(function_ptr[i].get_name()) - (string_info*)name_stack.get_base_addr()));
			if(!compile_info.extra_flag)
				function_ptr[i].debug_flag = 0;
			kfwrite(&function_ptr[i], sizeof(function_info), 1, file_ptr);
			if(this->compile_info.export_flag == EXPORT_FLAG_LINK || compile_info.extra_flag)
				function_ptr[i].set_name(((string_info*)name_stack.visit_element_by_index((int)function_ptr[i].get_name()))->get_name());
		}
	if(this->compile_info.extra_flag >= EXTRA_FLAG_DEBUG) {
		kfwrite(&this->compile_struct_info, sizeof(compile_struct_info_t), 1, file_ptr);
		count = this->compile_struct_info.struct_count;
		for(i=0; i<count; i++) {
			unsigned int arg_count = struct_ptr[i].varity_stack_ptr->get_count();
			kfwrite(struct_ptr[i].varity_stack_ptr, sizeof(stack), 1, file_ptr);
			for(int j=0; j<arg_count; j++) {
				varity_info* arg_varity_ptr = (varity_info*)struct_ptr[i].varity_stack_ptr->visit_element_by_index(j);
				PLATFORM_WORD *complex_ptr = arg_varity_ptr->get_complex_ptr();
				unsigned int type_no = varity_type_stack.find(arg_varity_ptr->get_complex_arg_count(), complex_ptr);
				arg_varity_ptr->config_complex_info(arg_varity_ptr->get_complex_arg_count(), (PLATFORM_WORD*)type_no);
				kfwrite(arg_varity_ptr, sizeof(varity_info), 1, file_ptr);
				arg_varity_ptr->config_complex_info(arg_varity_ptr->get_complex_arg_count(), complex_ptr);
			}
		}
		for(i=0; i<count; i++) {
			struct_ptr[i].set_name((char*)((string_info*)name_stack.find(struct_ptr[i].get_name()) - (string_info*)name_stack.get_base_addr()));
			kfwrite(&struct_ptr[i], sizeof(struct_info), 1, file_ptr);
			struct_ptr[i].set_name(((string_info*)name_stack.visit_element_by_index((int)struct_ptr[i].get_name()))->get_name());
		}
	}
	kfwrite(&this->compile_varity_info, sizeof(compile_varity_info_t), 1, file_ptr);
	if(this->compile_info.extra_flag >= EXTRA_FLAG_DEBUG) {
		kfwrite(&varity_type_stack, sizeof(varity_type_stack), 1, file_ptr);
		kfwrite(varity_type_stack.arg_count, 1, make_align(varity_type_stack.count, PLATFORM_WORD_LEN), file_ptr);
		for(i=0; i<varity_type_stack.count; i++) {
			for(j=varity_type_stack.arg_count[i]; j>=1; j--) {
				if(GET_COMPLEX_TYPE(varity_type_stack.type_info_addr[i][j]) == COMPLEX_BASIC && GET_COMPLEX_DATA(varity_type_stack.type_info_addr[i][j]) == STRUCT) {
					int struct_no = (struct_info*)varity_type_stack.type_info_addr[i][j - 1] - (struct_info*)this->struct_declare->struct_stack_ptr->get_base_addr();
					varity_type_stack.type_info_addr[i][j - 1] = struct_no;
				}
			}
			kfwrite(varity_type_stack.type_info_addr[i], PLATFORM_WORD_LEN, varity_type_stack.arg_count[i] + 1, file_ptr);
			for(j=varity_type_stack.arg_count[i]; j>=1; j--) {
				if(GET_COMPLEX_TYPE(varity_type_stack.type_info_addr[i][j]) == COMPLEX_BASIC && GET_COMPLEX_DATA(varity_type_stack.type_info_addr[i][j]) == STRUCT) {
					varity_type_stack.type_info_addr[i][j - 1] = (PLATFORM_WORD)this->struct_declare->struct_stack_ptr->visit_element_by_index(varity_type_stack.type_info_addr[i][j - 1]);
				}
			}			
		}
	}
	count = this->varity_declare->global_varity_stack->get_count();
	unsigned int varity_space_offset = 0;
	for(i=0; i<count; i++) {
		void *varity_space;
		if(!(varity_space = varity_info_ptr[i].get_content_ptr()))
			continue;
		int name_no = (string_info*)name_stack.find(varity_info_ptr[i].get_name()) - (string_info*)name_stack.get_base_addr();
		varity_info_ptr[i].set_name((char*)name_no);
		if(varity_info_ptr[i].get_complex_arg_count()) {
			varity_size = get_varity_size(0, varity_info_ptr[i].get_complex_ptr(), varity_info_ptr[i].get_complex_arg_count());
			varity_align_size = get_element_size(varity_info_ptr[i].get_complex_arg_count(), varity_info_ptr[i].get_complex_ptr());
		} else {
			varity_size = varity_info_ptr[i].get_size();
			varity_align_size = (unsigned int)varity_info_ptr[i].get_complex_ptr();
		}
		varity_space_offset = make_align(varity_space_offset, varity_align_size);
		varity_info_ptr[i].set_content_ptr((void*)varity_space_offset);
		varity_space_offset += varity_size;
		PLATFORM_WORD *complex_ptr = varity_info_ptr[i].get_complex_ptr();
		if(this->compile_info.extra_flag >= EXTRA_FLAG_DEBUG) {
			unsigned int type_no = varity_type_stack.find(varity_info_ptr[i].get_complex_arg_count(), complex_ptr);
			varity_info_ptr[i].config_complex_info(varity_info_ptr[i].get_complex_arg_count(), (PLATFORM_WORD*)type_no);
		} else {
			if(this->compile_info.export_flag == EXPORT_FLAG_LINK)
				varity_info_ptr[i].config_complex_info(0, (PLATFORM_WORD*)get_align_size(varity_info_ptr[i].get_complex_arg_count(), complex_ptr));
		}
		kfwrite(&varity_info_ptr[i], sizeof(varity_info), 1, file_ptr);
		varity_info_ptr[i].set_content_ptr(varity_space);
		varity_info_ptr[i].config_complex_info(varity_info_ptr[i].get_complex_arg_count(), complex_ptr);
	}
	for(i=0; i<count; i++) {
		//varity_size = get_varity_size(0, varity_info_ptr[i].get_complex_ptr(), varity_info_ptr[i].get_complex_arg_count());
		varity_size = varity_info_ptr[i].get_size();
		if(varity_info_ptr[i].get_content_ptr() && kmemchk(varity_info_ptr[i].get_content_ptr(), 0, varity_size))
			kfwrite(varity_info_ptr[i].get_content_ptr(), 1, varity_size, file_ptr);
	}
	kfclose(file_ptr);
	return ERROR_NO;
}

#define RETURN(x) if(avarity_use_flag && !PTR_N_VALUE(avarity_ptr->get_complex_ptr())) \
		vfree(avarity_ptr->get_complex_ptr()); \
	if(bvarity_use_flag && !PTR_N_VALUE(bvarity_ptr->get_complex_ptr())) \
		vfree(bvarity_ptr->get_complex_ptr()); \
	return x
int c_interpreter::operator_post_handle(stack *code_stack_ptr, node *opt_node_ptr)
{
	varity_info static_varity;
	register varity_info *avarity_ptr = &static_varity, *bvarity_ptr = &static_varity, *rvarity_ptr;
	int opt = ((node_attribute_t*)opt_node_ptr->value)->data;
	int varity_scope;
	register node_attribute_t *node_attribute;
	register mid_code *instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
	int varity_number;
	bool avarity_use_flag = 0, bvarity_use_flag = 0;
	int ret;
	int function_flag = 0;
	if(opt_number[opt] > 1) {
		node_attribute = (node_attribute_t*)opt_node_ptr->left->value;//判断是否是双目，单目不能看左树
		if(node_attribute->node_type == TOKEN_CONST_VALUE) {
			instruction_ptr->opda_operand_type = OPERAND_CONST;
			instruction_ptr->opda_varity_type = node_attribute->value_type;
			kmemcpy(&instruction_ptr->opda_addr, &node_attribute->value, 8);
		} else if(node_attribute->node_type == TOKEN_STRING) {
			instruction_ptr->opda_operand_type = OPERAND_STRING;
			instruction_ptr->opda_varity_type = ARRAY;
			kmemcpy(&instruction_ptr->opda_addr, &node_attribute->value, 8);
		} else if(node_attribute->node_type == TOKEN_NAME) {
			if(node_attribute->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {
				instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
				instruction_ptr->opda_addr = 8 * node_attribute->value.ptr_value[1];
				avarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]);
				instruction_ptr->opda_varity_type = avarity_ptr->get_type();
				this->interprete_need_ptr->mid_varity_stack.pop();
				avarity_use_flag = 1;
				dec_varity_ref(avarity_ptr, false);
			} else if(node_attribute->value.ptr_value[0] == LINK_VARITY_PREFIX) {
				varity_number = node_attribute->value.ptr_value[1];
				avarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
				instruction_ptr->opda_operand_type = OPERAND_LINK_VARITY;
				instruction_ptr->opda_varity_type = avarity_ptr->get_type();
				instruction_ptr->opda_addr = 8 * node_attribute->value.ptr_value[1];
				this->interprete_need_ptr->mid_varity_stack.pop();
				avarity_use_flag = 1;
				dec_varity_ref(avarity_ptr, false);
			} else {
				if(opt != OPT_CALL_FUNC) {
					avarity_ptr = this->varity_declare->vfind(node_attribute->value.ptr_value, varity_scope);
					if(!avarity_ptr) {
						tip_wrong(node_attribute->pos);
						error("Varity not exist.\n");
						return ERROR_VARITY_NONEXIST;
					}
					if(varity_scope == VARITY_SCOPE_GLOBAL) {
						instruction_ptr->opda_operand_type = OPERAND_G_VARITY;
						instruction_ptr->opda_addr = (int)avarity_ptr->get_name();
					} else {
						instruction_ptr->opda_operand_type = OPERAND_L_VARITY;
						instruction_ptr->opda_addr = (int)avarity_ptr->get_content_ptr();
					}
					
					instruction_ptr->opda_varity_type = avarity_ptr->get_type();
				}
			}
		}
	}
	if(opt_node_ptr->right) {
		node_attribute = (node_attribute_t*)opt_node_ptr->right->value;
		if(node_attribute->node_type == TOKEN_CONST_VALUE) {
			instruction_ptr->opdb_operand_type = OPERAND_CONST;
			instruction_ptr->opdb_varity_type = node_attribute->value_type;
			kmemcpy(&instruction_ptr->opdb_addr, &node_attribute->value, 8);
		} else if(node_attribute->node_type == TOKEN_STRING) {
			instruction_ptr->opdb_operand_type = OPERAND_STRING;
			instruction_ptr->opdb_varity_type = ARRAY;
			kmemcpy(&instruction_ptr->opdb_addr, &node_attribute->value, 8);
		} else if(node_attribute->node_type == TOKEN_NAME) {
			if(node_attribute->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {
				instruction_ptr->opdb_operand_type = OPERAND_T_VARITY;
				instruction_ptr->opdb_addr = 8 * node_attribute->value.ptr_value[1];
				bvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]);
				instruction_ptr->opdb_varity_type = bvarity_ptr->get_type();
				this->interprete_need_ptr->mid_varity_stack.pop();
				bvarity_use_flag = 1;
				dec_varity_ref(bvarity_ptr, false);
			} else if(node_attribute->value.ptr_value[0] == LINK_VARITY_PREFIX) {
				instruction_ptr->opdb_operand_type = OPERAND_LINK_VARITY;
				bvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]);
				instruction_ptr->opdb_varity_type = bvarity_ptr->get_type();
				instruction_ptr->opdb_addr = 8 * node_attribute->value.ptr_value[1];
				this->interprete_need_ptr->mid_varity_stack.pop();
				bvarity_use_flag = 1;
				dec_varity_ref(bvarity_ptr, false);
			} else {
				if(opt != OPT_MEMBER && opt != OPT_REFERENCE) {
					bvarity_ptr = this->varity_declare->vfind(node_attribute->value.ptr_value, varity_scope);
					if(!bvarity_ptr) {
						bvarity_ptr = (varity_info*)this->function_declare->find(node_attribute->value.ptr_value);
						if(!bvarity_ptr) {
							tip_wrong(node_attribute->pos);
							error("Varity not exist.\n");
							return ERROR_VARITY_NONEXIST;
						}
						instruction_ptr->opdb_operand_type = OPERAND_FUNCTION;
						instruction_ptr->opdb_varity_type = PTR;
						instruction_ptr->opdb_addr = (int)((function_info*)bvarity_ptr)->get_name();
						function_flag = 2;
					} else {
						if(varity_scope == VARITY_SCOPE_GLOBAL) {
							instruction_ptr->opdb_operand_type = OPERAND_G_VARITY;
							instruction_ptr->opdb_addr = (int)bvarity_ptr->get_name();
						} else {
							instruction_ptr->opdb_operand_type = OPERAND_L_VARITY;
							instruction_ptr->opdb_addr = (int)bvarity_ptr->get_content_ptr();
						}
						instruction_ptr->opdb_varity_type = bvarity_ptr->get_type();
					}
				}
			}
		}
	}

	switch(opt) {
		int ret_type;
	case OPT_MEMBER:
	case OPT_REFERENCE:
	{
		varity_info *member_varity_ptr;
		if(opt == OPT_MEMBER) {
			if(instruction_ptr->opda_varity_type != STRUCT) {
				tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
				error("Only struct can use . operator.\n");
				RETURN(ERROR_ILLEGAL_OPERAND);
			}
		} else {
			if(avarity_ptr->get_complex_arg_count() != 3 || GET_COMPLEX_DATA(((PLATFORM_WORD*)avarity_ptr->get_complex_ptr())[2]) != STRUCT || GET_COMPLEX_TYPE(((PLATFORM_WORD*)avarity_ptr->get_complex_ptr())[3]) != COMPLEX_PTR) {
				tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
				error("Only pointer to struct can use -> operator.\n");
				RETURN(ERROR_ILLEGAL_OPERAND);
			}
		}
		struct_info *struct_info_ptr = (struct_info*)(((PLATFORM_WORD*)avarity_ptr->get_complex_ptr())[1]);
		//ret_type = get_ret_type(instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type);
		varity_number = this->interprete_need_ptr->mid_varity_stack.get_count();
		member_varity_ptr = (varity_info*)struct_info_ptr->varity_stack_ptr->find(node_attribute->value.ptr_value);
		if(!member_varity_ptr) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("No member in struct.\n");
			RETURN(ERROR_STRUCT_MEMBER);
		}
		avarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
		avarity_ptr->config_complex_info(member_varity_ptr->get_complex_arg_count(), member_varity_ptr->get_complex_ptr());
		//avarity_ptr->set_type(member_varity_ptr->get_type());
		this->interprete_need_ptr->mid_varity_stack.push();
		inc_varity_ref(avarity_ptr);
		if(opt == OPT_MEMBER) {
			instruction_ptr->opda_varity_type = PLATFORM_TYPE;
			instruction_ptr->opdb_addr = (int)member_varity_ptr->get_content_ptr();
			instruction_ptr->opdb_operand_type = OPERAND_CONST;
			instruction_ptr->opdb_varity_type = INT;
			instruction_ptr->ret_operator = opt;
			instruction_ptr->ret_addr = varity_number * 8;
			instruction_ptr->ret_varity_type = PLATFORM_TYPE;
			instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		} else if(opt == OPT_REFERENCE) {
			instruction_ptr->opda_varity_type = PLATFORM_TYPE;
			instruction_ptr->opdb_addr = (int)member_varity_ptr->get_content_ptr();
			instruction_ptr->opdb_operand_type = OPERAND_CONST;
			instruction_ptr->opdb_varity_type = INT;
			instruction_ptr->ret_operator = OPT_PLUS;
			instruction_ptr->ret_addr = varity_number * 8;
			instruction_ptr->ret_varity_type = PLATFORM_TYPE;
			instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		}
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = link_varity_name[varity_number];
		break;
	}
	case OPT_INDEX:
	{
		varity_number = this->interprete_need_ptr->mid_varity_stack.get_count();
		PLATFORM_WORD *complex_info_ptr = avarity_ptr->get_complex_ptr();
		int complex_arg_count = avarity_ptr->get_complex_arg_count();
		if(GET_COMPLEX_TYPE(complex_info_ptr[complex_arg_count]) == COMPLEX_ARRAY || GET_COMPLEX_TYPE(complex_info_ptr[complex_arg_count]) == COMPLEX_PTR) {
			rvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
			rvarity_ptr->set_size(get_varity_size(GET_COMPLEX_DATA(complex_info_ptr[0]), complex_info_ptr, complex_arg_count - 1));
			rvarity_ptr->config_complex_info(complex_arg_count - 1, complex_info_ptr);
			this->interprete_need_ptr->mid_varity_stack.push();
			inc_varity_ref(rvarity_ptr);
			instruction_ptr->data = get_varity_size(0, complex_info_ptr, complex_arg_count - 1);
			instruction_ptr->ret_addr = varity_number * 8;
			instruction_ptr->ret_operand_type = OPERAND_LINK_VARITY;
			instruction_ptr->ret_operator = opt;
			//instruction_ptr->ret_varity_type = INT;
#if ARRAY_BOUND_CHECK
			if(GET_COMPLEX_TYPE(complex_info_ptr[complex_arg_count]) == COMPLEX_ARRAY) {
				int element_count = GET_COMPLEX_DATA(complex_info_ptr[complex_arg_count]);
				instruction_ptr->ret_varity_type = element_count & 0xFF;
				element_count >>= 8;
				instruction_ptr->opdb_varity_type = element_count & 0xFF;
				element_count >>= 8;
				instruction_ptr->data |= (element_count << 24);
			}
#endif
			((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
			((node_attribute_t*)opt_node_ptr->value)->value_type = instruction_ptr->opda_operand_type;
			((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = link_varity_name[varity_number];
		} else {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("No array or ptr varity for using [].\n");
			RETURN(ERROR_USED_INDEX);
		}
		break;
	}
	case OPT_PLUS:
	case OPT_MINUS:
	case OPT_MUL:
	case OPT_DIVIDE:
		if(opt == OPT_PLUS && instruction_ptr->opda_varity_type >= CHAR && instruction_ptr->opda_varity_type <= U_LONG_LONG && instruction_ptr->opdb_varity_type <= ARRAY && instruction_ptr->opdb_varity_type >= PTR) {
			char tmp[sizeof(instruction_ptr->opda_addr) + sizeof(instruction_ptr->opda_operand_type) + sizeof(instruction_ptr->double_space1) + sizeof(instruction_ptr->opda_varity_type)];
			kmemcpy(tmp, &instruction_ptr->opda_addr, sizeof(tmp));
			kmemcpy(&instruction_ptr->opda_addr, &instruction_ptr->opdb_addr, sizeof(tmp));
			kmemcpy(&instruction_ptr->opdb_addr, tmp, sizeof(tmp));
		}
		if(opt == OPT_PLUS)
			ret_type = try_call_opt_handle(OPT_PLUS, instruction_ptr, avarity_ptr, bvarity_ptr);
		else if(opt == OPT_MINUS)//TODO: 减法可用两个指针减，改用cmp的try_handle
			ret_type = try_call_opt_handle(OPT_MINUS, instruction_ptr, avarity_ptr, bvarity_ptr);
		else
			ret_type = try_call_opt_handle(OPT_MUL, instruction_ptr, avarity_ptr, bvarity_ptr);
		if(ret_type < 0) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("Can't use operator %s here\n", opt_str[opt]);
			RETURN(ret_type);
		}
		varity_number = this->interprete_need_ptr->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(ret_type);
		if(ret_type == PTR || ret_type == INT) {
			if(instruction_ptr->opda_varity_type >= PTR && instruction_ptr->opdb_varity_type <= VOID) {
				rvarity_ptr->config_complex_info(avarity_ptr->get_complex_arg_count(), avarity_ptr->get_complex_ptr());
				instruction_ptr->data = avarity_ptr->get_first_order_sub_struct_size();
			} else if(instruction_ptr->opda_varity_type <= VOID && instruction_ptr->opdb_varity_type >= PTR) {
				rvarity_ptr->config_complex_info(bvarity_ptr->get_complex_arg_count(), bvarity_ptr->get_complex_ptr());
				instruction_ptr->data = bvarity_ptr->get_first_order_sub_struct_size();
			} else if(instruction_ptr->opda_varity_type >= PTR && instruction_ptr->opdb_varity_type >= PTR) {
				rvarity_ptr->config_complex_info(bvarity_ptr->get_complex_arg_count(), bvarity_ptr->get_complex_ptr());
				instruction_ptr->data = bvarity_ptr->get_first_order_sub_struct_size();
			}
			array_to_ptr((PLATFORM_WORD*&)rvarity_ptr->get_complex_ptr(), rvarity_ptr->get_complex_arg_count());
		}
		this->interprete_need_ptr->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = ret_type;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		break;
	case OPT_EQU:
	case OPT_NOT_EQU:
	case OPT_BIG_EQU:
	case OPT_SMALL_EQU:
	case OPT_BIG:
	case OPT_SMALL:
		ret_type = try_call_opt_handle(OPT_EQU, instruction_ptr, avarity_ptr, bvarity_ptr);
		if(ret_type < 0) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("Can't use operator %s here\n", opt_str[opt]);
			RETURN(ret_type);
		}
		ret_type = INT;
		varity_number = this->interprete_need_ptr->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(ret_type);
		this->interprete_need_ptr->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = INT;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		break;
	case OPT_ASL_ASSIGN:
	case OPT_ASR_ASSIGN:
	case OPT_MOD_ASSIGN:
	case OPT_BIT_AND_ASSIGN:
	case OPT_BIT_XOR_ASSIGN:
	case OPT_BIT_OR_ASSIGN:
		if(instruction_ptr->opda_varity_type > U_LONG_LONG || instruction_ptr->opdb_varity_type > U_LONG_LONG) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("Need int type operand.\n");
			RETURN(ERROR_ILLEGAL_OPERAND);
		}
		goto assign_general;
	case OPT_ASSIGN:
		ret_type = try_call_opt_handle(OPT_ASSIGN, instruction_ptr, avarity_ptr, bvarity_ptr);
		if(ret_type < 0) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("Can't use operator %s here\n", opt_str[opt]);
			RETURN(ret_type);
		}
		goto assign_general;
	case OPT_MUL_ASSIGN:
	case OPT_DEVIDE_ASSIGN:
		ret_type = try_call_opt_handle(OPT_MUL, instruction_ptr, avarity_ptr, bvarity_ptr);
		if(ret_type < 0) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("Can't use operator %s here\n", opt_str[opt]);
			RETURN(ret_type);
		}
		goto assign_general;
	case OPT_ADD_ASSIGN:
		ret_type = try_call_opt_handle(OPT_PLUS, instruction_ptr, avarity_ptr, bvarity_ptr);
		if(ret_type < 0) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("Can't use operator %s here\n", opt_str[opt]);
			RETURN(ret_type);
		}
		if(ret_type == PTR) {
			if(instruction_ptr->opda_varity_type == PTR)
				goto assign_general;
			else {
				tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
				error("Can't plus assign.\n"); 
				RETURN(ret_type);
			}
		} else
			goto assign_general;
	case OPT_MINUS_ASSIGN:
		ret_type = try_call_opt_handle(OPT_MINUS, instruction_ptr, avarity_ptr, bvarity_ptr);
		if(ret_type < 0) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("Can't use operator %s here\n", opt_str[opt]);
			RETURN(ret_type);
		}
		if(ret_type == instruction_ptr->opda_varity_type)
			ret_type = try_call_opt_handle(OPT_ASSIGN, instruction_ptr, avarity_ptr, bvarity_ptr);
		else
			ret_type = try_call_opt_handle(OPT_ASSIGN, instruction_ptr, avarity_ptr, bvarity_ptr);
		if(ret_type < 0) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("Can't use operator %s here\n", opt_str[opt]);
			RETURN(ret_type);
		}
		goto assign_general;
assign_general:
		if(instruction_ptr->opda_operand_type == OPERAND_T_VARITY) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("Assign operator need left value.\n");
			RETURN(ERROR_NEED_LEFT_VALUE);
		} else if(instruction_ptr->opda_operand_type == OPERAND_CONST) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("Const value assigned.\n");
			RETURN(ERROR_CONST_ASSIGNED);
		} else {
		}
		if(ret_type == PTR || ret_type == INT) {
			if(instruction_ptr->opda_varity_type >= PTR && instruction_ptr->opdb_varity_type <= VOID) {
				instruction_ptr->data = avarity_ptr->get_first_order_sub_struct_size();
			} else if(instruction_ptr->opda_varity_type <= VOID && instruction_ptr->opdb_varity_type >= PTR) {
				instruction_ptr->data = bvarity_ptr->get_first_order_sub_struct_size();
			} else if(instruction_ptr->opda_varity_type >= PTR && instruction_ptr->opdb_varity_type >= PTR) {
				instruction_ptr->data = bvarity_ptr->get_first_order_sub_struct_size();
			}
		}
		instruction_ptr->ret_addr = instruction_ptr->opda_addr;
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type;
		instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = ((node_attribute_t*)opt_node_ptr->left->value)->value.ptr_value;
		if(instruction_ptr->opda_operand_type == OPERAND_T_VARITY || instruction_ptr->opda_operand_type == OPERAND_LINK_VARITY) {
			inc_varity_ref(avarity_ptr);
			this->interprete_need_ptr->mid_varity_stack.push();
		}
		break;
	case OPT_L_PLUS_PLUS:
	case OPT_L_MINUS_MINUS:
		if(instruction_ptr->opdb_operand_type == OPERAND_T_VARITY) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("++ operator need left value.\n");
			RETURN(ERROR_NEED_LEFT_VALUE);
		}
		instruction_ptr->opda_addr = instruction_ptr->opdb_addr;
		instruction_ptr->opda_operand_type = instruction_ptr->opdb_operand_type;
		instruction_ptr->opda_varity_type = instruction_ptr->opdb_varity_type;
		instruction_ptr->ret_addr = instruction_ptr->opda_addr;
		instruction_ptr->ret_operator = (opt == OPT_L_PLUS_PLUS) ? OPT_PLUS : OPT_MINUS;
		instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type;
		instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type;
		instruction_ptr->opdb_addr = 1;
		instruction_ptr->opdb_operand_type = OPERAND_CONST;
		instruction_ptr->opdb_varity_type = INT;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = ((node_attribute_t*)opt_node_ptr->right->value)->value.ptr_value;
		break;
	case OPT_R_PLUS_PLUS:
	case OPT_R_MINUS_MINUS:
		if(instruction_ptr->opdb_operand_type == OPERAND_T_VARITY) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("++ operator need left value.\n");
			RETURN(ERROR_NEED_LEFT_VALUE);
		}
		if(instruction_ptr->opdb_operand_type == OPERAND_CONST) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("++ operator cannot used for const.\n");
			RETURN(ERROR_CONST_ASSIGNED);
		}
		varity_number = this->interprete_need_ptr->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->config_complex_info(bvarity_ptr->get_complex_arg_count(), bvarity_ptr->get_complex_ptr());
		this->interprete_need_ptr->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = instruction_ptr->opda_addr = varity_number * 8;
		instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = instruction_ptr->opdb_varity_type;
		instruction_ptr->ret_operator = OPT_ASSIGN;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		code_stack_ptr->push();

		(instruction_ptr + 1)->ret_addr = instruction_ptr->opdb_addr;
		(instruction_ptr + 1)->ret_operator = (opt == OPT_R_PLUS_PLUS) ? OPT_PLUS : OPT_MINUS;
		(instruction_ptr + 1)->ret_operand_type = instruction_ptr->opdb_operand_type;
		(instruction_ptr + 1)->ret_varity_type = instruction_ptr->opdb_varity_type;
		(instruction_ptr + 1)->opda_addr = instruction_ptr->opdb_addr;
		(instruction_ptr + 1)->opda_operand_type = instruction_ptr->opdb_operand_type;
		(instruction_ptr + 1)->opda_varity_type = instruction_ptr->opdb_varity_type;
		(instruction_ptr + 1)->opdb_addr = 1;
		(instruction_ptr + 1)->opdb_operand_type = OPERAND_CONST;
		(instruction_ptr + 1)->opdb_varity_type = INT;
		if(instruction_ptr->opda_varity_type >= PTR) {
			(instruction_ptr + 1)->data = bvarity_ptr->get_first_order_sub_struct_size();
		}
		break;
	case OPT_ADDRESS_OF://TODO:取址时先验证扩展位是否为0再扩展，否则覆盖其他变量类型。
	{
		varity_number = this->interprete_need_ptr->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = PTR;
		instruction_ptr->ret_operator = opt;
		register PLATFORM_WORD* old_complex_info = (PLATFORM_WORD*)bvarity_ptr->get_complex_ptr();
		register int complex_arg_count = bvarity_ptr->get_complex_arg_count();
		if(GET_COMPLEX_TYPE(((uint*)bvarity_ptr->get_complex_ptr())[complex_arg_count + 1]) == COMPLEX_PTR) {
			rvarity_ptr->config_complex_info(complex_arg_count + 1, old_complex_info);
		} else {
			PLATFORM_WORD* new_complex_info = (PLATFORM_WORD*)dmalloc((complex_arg_count + 2) * sizeof(PLATFORM_WORD_LEN), "new varity type args");
			kmemcpy(new_complex_info + 1, old_complex_info + 1, complex_arg_count * sizeof(PLATFORM_WORD_LEN));
			new_complex_info[complex_arg_count + 1] = COMPLEX_PTR << COMPLEX_TYPE_BIT;
			rvarity_ptr->config_complex_info(complex_arg_count + 1, new_complex_info);
		}
		this->interprete_need_ptr->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		break;
	}
	case OPT_PTR_CONTENT:
	{
		varity_number = this->interprete_need_ptr->mid_varity_stack.get_count();
		//bvarity被rvarity覆盖，先处理，都要留意获取rvarity之后的覆盖问题
		rvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->config_complex_info(bvarity_ptr->get_complex_arg_count() - 1, bvarity_ptr->get_complex_ptr());
		rvarity_ptr->set_size(get_varity_size(0, bvarity_ptr->get_complex_ptr(), rvarity_ptr->get_complex_arg_count()));
		this->interprete_need_ptr->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_varity_type = rvarity_ptr->get_type();
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = OPERAND_LINK_VARITY;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = link_varity_name[varity_number];
		break;
	}
	case OPT_BIT_AND:
	case OPT_BIT_OR:
	case OPT_BIT_XOR:
	case OPT_MOD:
	case OPT_ASL:
	case OPT_ASR:
		ret_type = try_call_opt_handle(OPT_MOD, instruction_ptr, avarity_ptr, bvarity_ptr);
		if(ret_type < 0) {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("Can't use operator %s here\n", opt_str[instruction_ptr->ret_operator]);
			RETURN(ret_type);
		}
	case OPT_NOT:
	case OPT_BIT_REVERT:
	case OPT_POSITIVE:
	case OPT_NEGATIVE:
		varity_number = this->interprete_need_ptr->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
		if(opt == OPT_POSITIVE || opt == OPT_NEGATIVE) {
			rvarity_ptr->set_type(instruction_ptr->opdb_varity_type);
			instruction_ptr->ret_varity_type = instruction_ptr->opdb_varity_type;
		} else if(opt == OPT_MOD) {
			rvarity_ptr->set_type(ret_type);
			instruction_ptr->ret_varity_type = ret_type;
		} else {
			rvarity_ptr->set_type(INT);
			instruction_ptr->ret_varity_type = INT;
		}
		this->interprete_need_ptr->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		break;
	case OPT_TERNARY_Q:
	{
		varity_number = this->interprete_need_ptr->mid_varity_stack.get_count();
		instruction_ptr->ret_addr = instruction_ptr->opda_addr = varity_number * 8;
		instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = instruction_ptr->opdb_varity_type;
		instruction_ptr->ret_operator = OPT_ASSIGN;
		rvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(instruction_ptr->ret_varity_type);
		this->interprete_need_ptr->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		code_stack_ptr->push();
		instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
		instruction_ptr->ret_operator = CTL_BRANCH;
		mid_code *&ternary_code_ptr = (mid_code*&)this->interprete_need_ptr->sentence_analysis_data_struct.short_calc_stack[this->interprete_need_ptr->sentence_analysis_data_struct.short_depth - 1];
		ternary_code_ptr->opda_addr = ++instruction_ptr - ternary_code_ptr;
		this->interprete_need_ptr->sentence_analysis_data_struct.short_calc_stack[this->interprete_need_ptr->sentence_analysis_data_struct.short_depth - 1] = (PLATFORM_WORD)(instruction_ptr - 1);
		break;
	}
	case OPT_TERNARY_C:
	{
		varity_number = this->interprete_need_ptr->mid_varity_stack.get_count();
		instruction_ptr->ret_addr = instruction_ptr->opda_addr = varity_number * 8;
		instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = instruction_ptr->opdb_varity_type;
		instruction_ptr->ret_operator = OPT_ASSIGN;
		rvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(instruction_ptr->ret_varity_type);
		this->interprete_need_ptr->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		//code_stack_ptr->push();
		mid_code *&ternary_code_ptr = (mid_code*&)this->interprete_need_ptr->sentence_analysis_data_struct.short_calc_stack[--this->interprete_need_ptr->sentence_analysis_data_struct.short_depth];
		ternary_code_ptr->opda_addr = instruction_ptr + 1 - ternary_code_ptr;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		((varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number))->set_type(instruction_ptr->ret_varity_type);
		break;
	}
	case OPT_AND:
	case OPT_OR:
	{
		ret_type = INT;
		varity_number = this->interprete_need_ptr->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(ret_type);
		this->interprete_need_ptr->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		instruction_ptr->opda_operand_type = OPERAND_CONST;
		if(opt == OPT_AND)
			instruction_ptr->opda_addr = 1;
		else
			instruction_ptr->opda_addr = 0;
		instruction_ptr->ret_varity_type = INT;//get_ret_type(instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type);
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		//code_stack_ptr->push();
		instruction_ptr = (mid_code*)this->interprete_need_ptr->sentence_analysis_data_struct.short_calc_stack[--this->interprete_need_ptr->sentence_analysis_data_struct.short_depth];
		instruction_ptr->opda_addr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr() - instruction_ptr + 1;
		(instruction_ptr - 1)->opda_addr = varity_number * 8;
		break;
	}
	case OPT_COMMA:
	{
		node_attribute_t *&root_attribute_ptr = (node_attribute_t*&)opt_node_ptr->value;
		root_attribute_ptr->node_type = node_attribute->node_type;
		root_attribute_ptr->value = node_attribute->value;
		root_attribute_ptr->value_type = node_attribute->value_type;
		RETURN(ERROR_NO);
	}
	case OPT_CALL_FUNC:
	case OPT_FUNC_COMMA:
		if(opt_node_ptr->right) {
			node_attribute = (node_attribute_t*)opt_node_ptr->right->value;
			if(node_attribute->node_type == TOKEN_NAME || node_attribute->node_type == TOKEN_CONST_VALUE || node_attribute->node_type == TOKEN_STRING) {
				stack *arg_list_ptr = (stack*)this->call_func_info.function_ptr[this->call_func_info.function_depth - 1]->arg_list;
				instruction_ptr->ret_operator = OPT_PASS_PARA;
				instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type = OPERAND_L_VARITY;
#if HW_PLATFORM == PLATFORM_X86
				instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
#endif
				if(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] >= this->call_func_info.arg_count[this->call_func_info.function_depth - 1] - 1) {
					if(!this->call_func_info.function_ptr[this->call_func_info.function_depth - 1]->variable_para_flag) {
						tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
						error("Too many parameters\n");
						RETURN(ERROR_FUNC_ARGS);
					} else {//可变参数
						if(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] == this->call_func_info.arg_count[this->call_func_info.function_depth - 1]) {
							this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
						}
						if(instruction_ptr->opdb_varity_type <= U_LONG && instruction_ptr->opdb_varity_type >= CHAR) {//TODO:平台相关，应该换掉
							instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = INT;
#if HW_PLATFORM == PLATFORM_ARM
							instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
							this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4) + 4;
#else
							this->call_func_info.offset[this->call_func_info.function_depth - 1] += 4;
#endif
						} else if(instruction_ptr->opdb_varity_type == FLOAT || instruction_ptr->opdb_varity_type == DOUBLE) {
							instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = DOUBLE;
#if HW_PLATFORM == PLATFORM_ARM
							instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
							this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + 8;
#else
							this->call_func_info.offset[this->call_func_info.function_depth - 1] += 8;
#endif
						} else if(instruction_ptr->opdb_varity_type == PTR || instruction_ptr->opdb_varity_type == ARRAY) {
							instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = PLATFORM_TYPE;
#if HW_PLATFORM == PLATFORM_ARM
							instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
							this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) + PLATFORM_WORD_LEN;
#else
							this->call_func_info.offset[this->call_func_info.function_depth - 1] += PLATFORM_WORD_LEN;
#endif
						} else {
							instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = LONG_LONG;
#if HW_PLATFORM == PLATFORM_ARM
							instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
							this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + 8;
#else
							this->call_func_info.offset[this->call_func_info.function_depth - 1] += 8;
#endif
						}
					}
				} else {//确定参数
					varity_info *arg_varity_ptr = (varity_info*)(arg_list_ptr->visit_element_by_index(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] + 1));
#if HW_PLATFORM == PLATFORM_ARM
					switch(instruction_ptr->opdb_varity_type) {
					case DOUBLE:
					case LONG_LONG:
					case U_LONG_LONG:
						instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + arg_varity_ptr->get_size();
						break;
					case PTR:
					case ARRAY:
						instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) + arg_varity_ptr->get_size();
						break;
					default:
						instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4) + arg_varity_ptr->get_size();
						break;
					}
#else
					this->call_func_info.offset[this->call_func_info.function_depth - 1] += arg_varity_ptr->get_size();
#endif
					instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = arg_varity_ptr->get_type();
				}
				this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1]++;
				if(instruction_ptr->opdb_operand_type == OPERAND_T_VARITY || instruction_ptr->opdb_operand_type == OPERAND_LINK_VARITY) {
					//this->interprete_need_ptr->mid_varity_stack.pop();//TODO:加回来肯定有bug，不加回来不知道有没有
					//dec_varity_ref(bvarity_ptr, true);//TODO:确认是否有需要，不注释掉出bug了，释放了全局变量类型信息
				}
			}
		}
		if(opt == OPT_CALL_FUNC) {
			varity_info *return_varity_ptr;
			node_attribute = (node_attribute_t*)opt_node_ptr->left->value;
			function_info *function_ptr;
			function_ptr = this->function_declare->find(node_attribute->value.ptr_value);
			if(opt_node_ptr->right && !(((node_attribute_t*)opt_node_ptr->right->value)->data == OPT_FUNC_COMMA && ((node_attribute_t*)opt_node_ptr->right->value)->node_type == TOKEN_OPERATOR))
				code_stack_ptr->push();
			instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
			instruction_ptr->ret_operator = CTL_SP_ADD;
			instruction_ptr++->opda_addr = make_align((PLATFORM_WORD)this->varity_declare->local_varity_stack->offset, 4) + this->call_func_info.para_offset;
			code_stack_ptr->push();
			return_varity_ptr = (varity_info*)function_ptr->arg_list->visit_element_by_index(0);
			instruction_ptr->opdb_operand_type = OPERAND_CONST;
			instruction_ptr->opda_addr = (int)function_ptr->get_name();
			instruction_ptr->ret_operator = opt;
			instruction_ptr->ret_varity_type = return_varity_ptr->get_type();
			varity_number = this->interprete_need_ptr->mid_varity_stack.get_count();
			instruction_ptr->ret_addr = varity_number * 8;
			if(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] < this->call_func_info.arg_count[this->call_func_info.function_depth - 1] - 1) {
				error("Insufficient parameters for %s.\n", node_attribute->value.ptr_value);
				RETURN(ERROR_FUNC_ARGS);
			}
			instruction_ptr->opda_varity_type = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) / PLATFORM_WORD_LEN;
			instruction_ptr->data = this->varity_declare->local_varity_stack->offset;
			instruction_ptr->opdb_addr = this->interprete_need_ptr->mid_varity_stack.get_count() * 8;//TODO:==ret_addr? use opdb to log struct return size.
			instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
			((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
			((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
			rvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
			rvarity_ptr->config_complex_info(return_varity_ptr->get_complex_arg_count(), return_varity_ptr->get_complex_ptr());
			inc_varity_ref(rvarity_ptr);
			this->interprete_need_ptr->mid_varity_stack.push();
			this->call_func_info.function_depth--;
			code_stack_ptr->push();
			(++instruction_ptr)->ret_operator = CTL_SP_ADD;
			instruction_ptr->opda_addr = -(make_align((PLATFORM_WORD)this->varity_declare->local_varity_stack->offset, 4) + this->call_func_info.para_offset);
			if(this->call_func_info.function_depth) {
				this->call_func_info.para_offset -= make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4) + PLATFORM_WORD_LEN;
			}
		}
		break;
	case OPT_TYPE_CONVERT:
		varity_number = this->interprete_need_ptr->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->config_complex_info(((node_attribute_t*)opt_node_ptr->value)->count, (PLATFORM_WORD*)((node_attribute_t*)opt_node_ptr->value)->value.ptr_value);
		this->interprete_need_ptr->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = instruction_ptr->opda_addr = varity_number * 8;
		instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = rvarity_ptr->get_type();
		instruction_ptr->ret_operator = OPT_ASSIGN;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];		
		break;
	case OPT_EDGE:
		error("Extra ;\n");
		RETURN(ERROR_SEMICOLON);
	case OPT_EXIST:
		varity_number = this->interprete_need_ptr->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(INT);
		this->interprete_need_ptr->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = INT;
		instruction_ptr->ret_operator = OPT_EXIST;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];		
		break;
	case OPT_SIZEOF:
		--this->interprete_need_ptr->sentence_analysis_data_struct.sizeof_depth;
		while(this->cur_mid_code_stack_ptr->get_count() > this->interprete_need_ptr->sentence_analysis_data_struct.sizeof_code_count[this->interprete_need_ptr->sentence_analysis_data_struct.sizeof_depth]) {
			this->cur_mid_code_stack_ptr->pop();
		}
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_CONST_VALUE;
		((node_attribute_t*)opt_node_ptr->value)->value_type = U_INT;
		if(((node_attribute_t*)opt_node_ptr->right->value)->node_type == TOKEN_NAME) {
			if(((node_attribute_t*)opt_node_ptr->right->value)->value.ptr_value[0] == TMP_VAIRTY_PREFIX || ((node_attribute_t*)opt_node_ptr->right->value)->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {
				varity_number = ((node_attribute_t*)opt_node_ptr->right->value)->value.ptr_value[1];
				rvarity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(varity_number);
			} else
				rvarity_ptr = (varity_info*)this->varity_declare->find(((node_attribute_t*)opt_node_ptr->right->value)->value.ptr_value);
			if(rvarity_ptr) {
				((node_attribute_t*)opt_node_ptr->value)->value.int_value = get_varity_size(0, rvarity_ptr->get_complex_ptr(), rvarity_ptr->get_complex_arg_count());
			} else {
				tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
				error("Varity not exist.\n");
				RETURN(ERROR_VARITY_NONEXIST);
			}
		} else if(((node_attribute_t*)opt_node_ptr->right->value)->node_type == TOKEN_CONST_VALUE) {

		} else if(((node_attribute_t*)opt_node_ptr->right->value)->node_type == TOKEN_OPERATOR && ((node_attribute_t*)opt_node_ptr->right->value)->data == OPT_TYPE_CONVERT) {
			((node_attribute_t*)opt_node_ptr->value)->value.int_value = get_varity_size(0, (PLATFORM_WORD*)((node_attribute_t*)opt_node_ptr->right->value)->value.ptr_value, ((node_attribute_t*)opt_node_ptr->right->value)->count);
		} else {
			tip_wrong(((node_attribute_t*)opt_node_ptr->value)->pos);
			error("Wrong use of sizeof.\n");
			RETURN(ERROR_SIZEOF);
		}
		RETURN(ERROR_NO);
	default:
		error("what??\n");
		break;
	}
	code_stack_ptr->push();
	RETURN(ERROR_NO);
}
#undef RETURN

int c_interpreter::operator_mid_handle(stack *code_stack_ptr, node *opt_node_ptr)
{
	register mid_code *instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
	node_attribute_t *node_attribute = ((node_attribute_t*)opt_node_ptr->value);//避免滥用&，误修改了变量。
	switch(node_attribute->data) {
		varity_info *varity_ptr;
	case OPT_TERNARY_Q:
		this->generate_expression_value(code_stack_ptr, (node_attribute_t*)opt_node_ptr->left->value);
		this->interprete_need_ptr->mid_varity_stack.pop();
		varity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(this->interprete_need_ptr->mid_varity_stack.get_count());
		dec_varity_ref(varity_ptr, true);
		((node_attribute_t*)opt_node_ptr->left->value)->node_type = TOKEN_CONST_VALUE;
		instruction_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		instruction_ptr->ret_operator = CTL_BRANCH_FALSE;
		this->interprete_need_ptr->sentence_analysis_data_struct.short_calc_stack[this->interprete_need_ptr->sentence_analysis_data_struct.short_depth++] = (PLATFORM_WORD)instruction_ptr;
		code_stack_ptr->push();
		break;
	case OPT_CALL_FUNC:
		this->call_func_info.function_ptr[this->call_func_info.function_depth] = this->function_declare->find(((node_attribute_t*)opt_node_ptr->left->value)->value.ptr_value);
		if(!this->call_func_info.function_ptr[this->call_func_info.function_depth])
			this->call_func_info.function_ptr[this->call_func_info.function_depth] = (function_info*)this->varity_declare->find(((node_attribute_t*)opt_node_ptr->left->value)->value.ptr_value)->get_content_ptr();
		this->call_func_info.arg_count[this->call_func_info.function_depth] = this->call_func_info.function_ptr[this->call_func_info.function_depth]->arg_list->get_count();
		this->call_func_info.cur_arg_number[this->call_func_info.function_depth] = 0;
		this->call_func_info.offset[this->call_func_info.function_depth] = 0;
		if(this->call_func_info.function_depth) {
			this->call_func_info.para_offset += make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4) + PLATFORM_WORD_LEN;
		}
		this->call_func_info.function_depth++;
		break;
	case OPT_FUNC_COMMA:
		if(((node_attribute_t*)opt_node_ptr->left->value)->node_type == TOKEN_NAME ||  ((node_attribute_t*)opt_node_ptr->left->value)->node_type == TOKEN_CONST_VALUE || ((node_attribute_t*)opt_node_ptr->left->value)->node_type == TOKEN_STRING) {
			stack *arg_list_ptr = (stack*)this->call_func_info.function_ptr[this->call_func_info.function_depth - 1]->arg_list;
			node_attribute = (node_attribute_t*)opt_node_ptr->left->value;
			if(node_attribute->node_type == TOKEN_CONST_VALUE) {
				instruction_ptr->opdb_operand_type = OPERAND_CONST;
				instruction_ptr->opdb_varity_type = node_attribute->value_type;
				kmemcpy(&instruction_ptr->opdb_addr, &node_attribute->value, 8);
			} else if(node_attribute->node_type == TOKEN_STRING) {
				instruction_ptr->opdb_operand_type = OPERAND_STRING;
				instruction_ptr->opdb_varity_type = ARRAY;
				instruction_ptr->opdb_addr = (int)node_attribute->value.ptr_value;
			} else if(node_attribute->node_type == TOKEN_NAME) {
				if(node_attribute->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {
					instruction_ptr->opdb_operand_type = OPERAND_T_VARITY;
					instruction_ptr->opdb_addr = 8 * node_attribute->value.ptr_value[1];
					instruction_ptr->opdb_varity_type = ((varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]))->get_type();
				} else {
					int varity_scope;
					varity_ptr = this->varity_declare->vfind(node_attribute->value.ptr_value, varity_scope);
					if(!varity_ptr) {
						tip_wrong(node_attribute->pos);
						error("Varity not exist.\n");
						return ERROR_VARITY_NONEXIST;
					}
					if (varity_scope == VARITY_SCOPE_GLOBAL) {
						instruction_ptr->opdb_operand_type = OPERAND_G_VARITY;
						instruction_ptr->opdb_addr = (int)varity_ptr->get_name();
					} else {
						instruction_ptr->opdb_operand_type = OPERAND_L_VARITY;
						instruction_ptr->opdb_addr = (int)varity_ptr->get_content_ptr();
					}
					
					instruction_ptr->opdb_varity_type = varity_ptr->get_type();
				}
			}
			varity_info *arg_varity_ptr;
			instruction_ptr->ret_operator = OPT_PASS_PARA;
			instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type = OPERAND_L_VARITY;
#if HW_PLATFORM == PLATFORM_X86
			instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
#endif
			if(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] >= this->call_func_info.arg_count[this->call_func_info.function_depth - 1] - 1) {
				if(!this->call_func_info.function_ptr[this->call_func_info.function_depth - 1]->variable_para_flag) {
					error("Too many parameters\n");
					return ERROR_FUNC_ARGS;
				} else {
					if(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] == this->call_func_info.arg_count[this->call_func_info.function_depth - 1]) {
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
					}
					if(instruction_ptr->opdb_varity_type <= U_LONG && instruction_ptr->opdb_varity_type >= CHAR) {
						instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = INT;
#if HW_PLATFORM == PLATFORM_ARM
						instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
						this->call_func_info.offset[this->call_func_info.function_depth - 1] += 4;
#else
						this->call_func_info.offset[this->call_func_info.function_depth - 1] += 4;
#endif
					} else if(instruction_ptr->opdb_varity_type == FLOAT || instruction_ptr->opdb_varity_type == DOUBLE) {
						instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = DOUBLE;
#if HW_PLATFORM == PLATFORM_ARM
						instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + 8;
#else
						this->call_func_info.offset[this->call_func_info.function_depth - 1] += 8;
#endif
					} else if(instruction_ptr->opdb_varity_type == PTR || instruction_ptr->opdb_varity_type == ARRAY) {
						instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = PLATFORM_TYPE;
#if HW_PLATFORM == PLATFORM_ARM
						instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) + PLATFORM_WORD_LEN;
#else
						this->call_func_info.offset[this->call_func_info.function_depth - 1] += PLATFORM_WORD_LEN;
#endif
					} else {
						instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = LONG_LONG;
#if HW_PLATFORM == PLATFORM_ARM
						instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + 8;
#else
						this->call_func_info.offset[this->call_func_info.function_depth - 1] += 8;
#endif
					}
				}
			} else {
				arg_varity_ptr = (varity_info*)(arg_list_ptr->visit_element_by_index(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] + 1));
#if HW_PLATFORM == PLATFORM_ARM
				switch(instruction_ptr->opdb_varity_type) {
				case DOUBLE:
				case LONG_LONG:
				case U_LONG_LONG:
					instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
					this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + arg_varity_ptr->get_size();
					break;
				case PTR:
				case ARRAY:
					instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
					this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) + arg_varity_ptr->get_size();
					break;
				default:
					instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4) + make_align(this->varity_declare->local_varity_stack->offset, PLATFORM_WORD_LEN) + this->call_func_info.para_offset;
					this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4) + arg_varity_ptr->get_size();
					break;
				}
#else
				this->call_func_info.offset[this->call_func_info.function_depth - 1] += arg_varity_ptr->get_size();
#endif
				instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = arg_varity_ptr->get_type();
			}
			this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1]++;
			code_stack_ptr->push();
		}
		break;
	case OPT_AND://TODO:和OR合并
	case OPT_OR:
		this->generate_expression_value(code_stack_ptr, (node_attribute_t*)opt_node_ptr->left->value);
		this->interprete_need_ptr->mid_varity_stack.pop();
		instruction_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		if(node_attribute->data == OPT_AND)
			instruction_ptr->ret_operator = CTL_BRANCH_FALSE;
		else
			instruction_ptr->ret_operator = CTL_BRANCH_TRUE;
		this->interprete_need_ptr->sentence_analysis_data_struct.short_calc_stack[this->interprete_need_ptr->sentence_analysis_data_struct.short_depth++] = (PLATFORM_WORD)instruction_ptr;
		code_stack_ptr->push();
		break;
	}
	return ERROR_NO;
}

int c_interpreter::operator_pre_handle(stack *code_stack_ptr, node *opt_node_ptr)
{
	register mid_code *instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
	node_attribute_t *node_attribute = ((node_attribute_t*)opt_node_ptr->value);
	switch(node_attribute->data) {
	case OPT_TERNARY_C:
		if(((node_attribute_t*)opt_node_ptr->left->value)->data != OPT_TERNARY_Q) {
			error("? && : unmatch.\n");
			return ERROR_TERNARY_UNMATCH;
		}
	case OPT_SIZEOF:
		this->interprete_need_ptr->sentence_analysis_data_struct.sizeof_code_count[this->interprete_need_ptr->sentence_analysis_data_struct.sizeof_depth++] = this->cur_mid_code_stack_ptr->get_count();
		break;
	}
	return ERROR_NO;
}

int c_interpreter::tree_to_code(node *tree, stack *code_stack)
{
	int ret;
	if(((node_attribute_t*)tree->value)->node_type == TOKEN_OPERATOR) {	//需要先序处理的运算符：TERNERY_C
		ret = this->operator_pre_handle(code_stack, tree);
		if(ret)
			return ret;
	}
	if(tree->left && ((node_attribute_t*)tree->left->value)->node_type == TOKEN_OPERATOR) {
		ret = this->tree_to_code(tree->left, code_stack);
		if(ret)
			return ret;
	}
	if(((node_attribute_t*)tree->value)->node_type == TOKEN_OPERATOR) {	//需要中序处理的几个运算符：CALL_FUNC，&&，||，FUNC_COMMA等
		ret = this->operator_mid_handle(code_stack, tree);
		if(ret)
			return ret;
	}
	if(tree->right && ((node_attribute_t*)tree->right->value)->node_type == TOKEN_OPERATOR) {
		ret = this->tree_to_code(tree->right, code_stack);
		if(ret)
			return ret;
	}
	if(((node_attribute_t*)tree->value)->node_type == TOKEN_OPERATOR) {
		ret = this->operator_post_handle(code_stack, tree);
		if(ret)
			return ret;
	}
	return ERROR_NO;
}

int c_interpreter::generate_token_list(char *str, uint len)
{
	int token_len;
	node_attribute_t *raw_token_ptr = (node_attribute_t*)this->interprete_need_ptr->mid_code_stack.get_current_ptr();
	int i = 0;
	char *strbeg = str;
	while(len > 0) {
		token_len = get_token(str, &raw_token_ptr[i]);
		if(token_len < 0)
			return token_len;
		raw_token_ptr[i].pos = str - strbeg;
		len -= token_len;
		str += token_len;
		if(raw_token_ptr[i].node_type != TOKEN_NONEXIST)
			i++;
		else
			break;
	}
	token_len = this->token_convert(raw_token_ptr, i);
	return token_len;
}

int c_interpreter::find_fptr_by_code(mid_code *mid_code_ptr, function_info *&fptr, int *line_ptr)
{
	if(mid_code_ptr >= (mid_code*)this->interprete_need_ptr->mid_code_stack.get_base_addr() && mid_code_ptr < (mid_code*)this->interprete_need_ptr->mid_code_stack.get_base_addr() + MAX_MID_CODE_COUNT) {
		fptr = 0;
		if(line_ptr) {
			int j = this->nonseq_info->row_num - 1;
			if(j < 0)
				j = 0;
			do {
#if DEBUG_EN
				if(this->nonseq_info->row_info_node[j].row_code_ptr <= mid_code_ptr) {
					*line_ptr = j;
					break;
				}
#endif
			} while(--j >= 0);
		}
		return ERROR_NO;
	} else {
		int i;
		function_info *function_base = (function_info*)this->function_declare->function_stack_ptr->get_base_addr();
		mid_code *func_pc;
		int func_code_count;
		for(i=0; i<this->language_elment_space.function_list.get_count(); i++) {
			func_pc = (mid_code*)function_base[i].mid_code_stack.get_base_addr();
			func_code_count = function_base[i].mid_code_stack.get_count();
			if(mid_code_ptr >= func_pc && mid_code_ptr < func_pc + func_code_count) {
				fptr = &function_base[i];
				if(!fptr->row_code_ptr)
					continue;
#if DEBUG_EN
				if(fptr->debug_flag) {
					if(line_ptr) {
						for(int j=fptr->row_line-1; j>=0; j--) {
							if(fptr->row_code_ptr[j] <= mid_code_ptr) {
								*line_ptr = j;
								break;
							}
						}
					}
				} else {
					*line_ptr = -1;
				}
#endif
				return ERROR_NO;
			}
		}
		if(i == this->language_elment_space.function_list.get_count()) {
			return ERROR_ILLEGAL_CODE;
		}
	}
}

int c_interpreter::print_call_stack(void)
{
	char *stack_ptr = this->stack_pointer;
	mid_code *pc, *func_pc;
	int func_code_count;
	pc = this->pc;
	function_info *function_base = (function_info*)this->function_declare->function_stack_ptr->get_base_addr();
	gdbout("Print call stack:\n");
	if(pc >= (mid_code*)this->interprete_need_ptr->mid_code_stack.get_base_addr() && pc < (mid_code*)this->interprete_need_ptr->mid_code_stack.get_base_addr() + MAX_MID_CODE_COUNT) {
		gdbout("main + %d\n", pc - (mid_code*)this->interprete_need_ptr->mid_code_stack.get_base_addr());
	} else {
		int i;
		for(i=0; i<this->language_elment_space.function_list.get_count(); i++) {
			func_pc = (mid_code*)function_base[i].mid_code_stack.get_base_addr();
			func_code_count = function_base[i].mid_code_stack.get_count();
			if(pc >= func_pc && pc < func_pc + func_code_count) {
				gdbout("%s + %d\n", function_base[i].get_name(), pc - func_pc);
				break;
			}
		}
		if(i == this->language_elment_space.function_list.get_count()) {
			error("stack broken.\n");
			return ERROR_NO;
		}
	}
	while(stack_ptr > this->simulation_stack + PLATFORM_WORD_LEN) {
		pc = (mid_code*)PTR_N_VALUE(stack_ptr - PLATFORM_WORD_LEN);
		if(pc >= (mid_code*)this->interprete_need_ptr->mid_code_stack.get_base_addr() && pc < (mid_code*)this->interprete_need_ptr->mid_code_stack.get_base_addr() + MAX_MID_CODE_COUNT) {
			gdbout("main + %d\n", pc - (mid_code*)this->interprete_need_ptr->mid_code_stack.get_base_addr());
			break;
		} else {
			int i;
			for(i=0; i<this->language_elment_space.function_list.get_count(); i++) {
				func_pc = (mid_code*)function_base[i].mid_code_stack.get_base_addr();
				func_code_count = function_base[i].mid_code_stack.get_count();
				if(pc >= func_pc && pc < func_pc + func_code_count) {
					gdbout("%s + %d\n", function_base[i].get_name(), pc - func_pc);
					stack_ptr -= pc->data + PLATFORM_WORD_LEN;
					break;
				}
			}
			if(i == this->language_elment_space.function_list.get_count()) {
				error("stack broken.\n");
				break;
			}
		}
	}
	return ERROR_NO;
}

int c_interpreter::preprocess(char *str, int &len)
{
	int ret;
	node_attribute_t node;
	macro_info *macro_ptr;
	if(str[0] == '#') {
		str++;
		if(!kstrncmp(str, "ifdef", 5) && kisspace(str[5])) {
			str += 6;
			ret = get_token(str, &node);
			if(node.node_type == TOKEN_NAME) {
				macro_ptr = this->macro_declare->find(node.value.ptr_value);
				preprocess_info.ifdef_level++;
				if(macro_ptr) {
					preprocess_info.ifdef_status[preprocess_info.ifdef_level - 1] = 0;//0:if命中 1:if失败
				} else {
					preprocess_info.ifdef_status[preprocess_info.ifdef_level - 1] = 1;
				}
				//if(preprocess_info.ifdef_level > 1)
				//	preprocess_info.ifdef_status[preprocess_info.ifdef_level - 1] &= ~preprocess_info.ifdef_status[preprocess_info.ifdef_level - 2];
			} else {
				error("no macro name\n");
				return ERROR_PREPROCESS;
			}
		} else if(!kstrncmp(str, "ifndef", 6) && kisspace(str[6])) {
			str += 7;
			ret = get_token(str, &node);
			if(node.node_type == TOKEN_NAME) {
				macro_ptr = this->macro_declare->find(node.value.ptr_value);
				preprocess_info.ifdef_level++;
				if(macro_ptr) {
					preprocess_info.ifdef_status[preprocess_info.ifdef_level - 1] = 1;//0:if命中 1:if失败
				} else {
					preprocess_info.ifdef_status[preprocess_info.ifdef_level - 1] = 0;
				}
				//if(preprocess_info.ifdef_level > 1)
				//	preprocess_info.ifdef_status[preprocess_info.ifdef_level - 1] &= ~preprocess_info.ifdef_status[preprocess_info.ifdef_level - 2];
			} else {
				error("no macro name\n");
				return ERROR_PREPROCESS;
			}
		} else if(!kstrncmp(str, "else", 4)) {
			preprocess_info.ifdef_status[preprocess_info.ifdef_level - 1] = !preprocess_info.ifdef_status[preprocess_info.ifdef_level - 1];
			//if(preprocess_info.ifdef_level > 1)
			//	preprocess_info.ifdef_status[preprocess_info.ifdef_level - 1] &= ~preprocess_info.ifdef_status[preprocess_info.ifdef_level - 2];
		} else if(!kstrncmp(str, "endif", 5)) {
			if(preprocess_info.ifdef_level)
				preprocess_info.ifdef_level--;
			else {
				error("no #ifdef/ifndef corresponding\n");
				return ERROR_PREPROCESS;
			}
		} else {
			str--;
			goto preprocess_noif;
		}
		return OK_PREPROCESS;
	} else {
preprocess_noif:
		int index = 0;
		if(preprocess_info.ifdef_level) {
			int ifdefret = 0;
			int i;
			for(i=0; i<preprocess_info.ifdef_level; i++) {
				if(preprocess_info.ifdef_status[i]) {
					ifdefret = 1;
					break;
				}
			}
			if(ifdefret) {
				str[0] = 0;
				len = 0;
				return ERROR_NO;
			}
		}
		if(str[0] == '#') {
			str++;
			if(!kstrncmp(str, "define", 6) && kisspace(str[6])) {
				int para_count = 0;
				int status = 0;
				str += 7;
				ret = get_token(str, &node);
				if(node.node_type == TOKEN_NAME) {
					macro_info macro_info;
					kmemset(macro_info.macro_arg_name, 0, sizeof(macro_info.macro_arg_name));
					macro_info.set_name(node.value.ptr_value);
					str += ret;
					ret = get_token(str, &node);
					if(node.node_type == TOKEN_OPERATOR && node.data == OPT_L_SMALL_BRACKET && node.pos == 0) {
						while(1) {
							str += ret;
							ret = get_token(str, &node);
							if(status == 0) {
								if(node.node_type == TOKEN_NAME) {
									status = 1;
									macro_info.macro_arg_name[para_count] = node.value.ptr_value;
								} else {
									error("Macro define error\n");
									return ERROR_MACRO_DEF;
								}
							} else {
								if(node.node_type == TOKEN_OPERATOR) {
									if(node.data == OPT_COMMA) {
										status = 0;
										if(++para_count > MAX_MACRO_ARG_COUNT) {
											error("Macro parameters too much.\n");
											return ERROR_MACRO_DEF;
										}
									} else if(node.data == OPT_R_SMALL_BRACKET) {
										str += ret;
										break;
									}
								} else {
									error("Macro define error\n");
									return ERROR_MACRO_DEF;
								}
							}
						}
						while(*str == ' ' || *str == '\t') str++;
						macro_info.macro_instead_str = (char*)dmalloc(kstrlen(str) + 1, "macro string");
						kstrcpy(macro_info.macro_instead_str, str);
					} else {
						while(*str == ' ' || *str == '\t') str++;
						macro_info.macro_instead_str = (char*)dmalloc(kstrlen(str) + 1, "macro string");
						kstrcpy(macro_info.macro_instead_str, str);
					}
					this->macro_declare->declare(macro_info.get_name(), macro_info.macro_arg_name, macro_info.macro_instead_str);
				}
			} else if(!kstrncmp(str, "include", 7) && kisspace(str[7])) {
				str += 8;
				ret = get_token(str, &node);
				if(node.node_type == TOKEN_STRING) {
					this->open_ref(((string_info*)string_stack.visit_element_by_index(node.value.int_value))->get_name());
					return OK_PREPROCESS;
				} else {
					error("#include error");
					return ERROR_PREPROCESS;
				}
			}
			return OK_PREPROCESS;
		}
		while(1) {
			int delta_len;
			ret = get_token(str + index, &node);
			if(node.node_type == TOKEN_NONEXIST || !ret)
				break;
			if(node.node_type == TOKEN_NAME) {
				macro_ptr = this->macro_declare->find(node.value.ptr_value);
				if(macro_ptr) {
					if(!macro_ptr->macro_arg_name[0]) {//no para macro
						delta_len = sub_replace(str, index, ret, macro_ptr->macro_instead_str);
						index += ret + delta_len;
						len += delta_len;
					} else {
						int extra_len = get_token(str + index + ret, &node);
						int para_begin_pos[3] = {0}, para_end_pos;
						int para_count = 0;
						int i, j, pos, cur_level;
						char *macro_para = str + index + ret + extra_len;
						if(node.node_type == TOKEN_OPERATOR && node.data == OPT_L_SMALL_BRACKET) {
							char *expand_macro = (char*)dmalloc(3 * kstrlen(macro_ptr->macro_instead_str), "macro expand");
							//TODO: 判断宏展开长度是否过长，超出分配空间
							for(i=0, cur_level=0; macro_para[i]; i++) {
								if((macro_para[i] == ',' || macro_para[i] == ')') && 0 == cur_level) {
									if(++para_count > MAX_MACRO_ARG_COUNT) {
										vfree(expand_macro);
										error("too much macro parameters.\n");
										return ERROR_MACRO_EXP;
									}
									if(para_count < MAX_MACRO_ARG_COUNT)
										para_begin_pos[para_count] = i + 1;
									if(macro_para[i] == ')') {
										macro_para[i] = 0;
										para_end_pos = i;
										break;
									}
									macro_para[i] = 0;
								} else if(macro_para[i] == '(' || macro_para[i] == '[') {
									cur_level++;
								} else if(macro_para[i] == ')' || macro_para[i] == ']') {
									cur_level--;
								}
							}
							
							for(i=0, pos=0;;) {
								ret = get_token(macro_ptr->macro_instead_str + i, &node);
								if(!ret || node.node_type == TOKEN_NONEXIST)
									break;
								if(node.node_type == TOKEN_NAME && (j = macro_ptr->find(node.value.ptr_value)) >= 0) {
									int strlen = kstrlen(&macro_para[para_begin_pos[j]]);
									kmemcpy(expand_macro + pos, macro_para + para_begin_pos[j], strlen); 
									pos += strlen;
									i += ret;
								} else {
									kmemcpy(expand_macro + pos,  macro_ptr->macro_instead_str + i, ret);
									i += ret;
									pos += ret;
								}
							}
							expand_macro[pos] = 0;
							delta_len = sub_replace(str, index, &macro_para[para_end_pos] - (str + index) + 1, expand_macro);
							index += pos;//ret + delta_len;
							len += ret + delta_len;
							vfree(expand_macro);
						} else {
							error("error macro %s\n", macro_ptr->get_name());
							return ERROR_MACRO_EXP;
						}
					}
				} else {
					index += ret;
				}
			} else {
				index += ret;
			}
		}
		return ERROR_NO;
	}
}

int c_interpreter::pre_treat(char *str, uint len)
{
	int spacenum = 0;
	int rptr = 0, wptr = 0, first_word = 1, space_flag = 0;
	int string_flag = 0;
	char bracket_stack[32], bracket_depth = 0;
	interprete_need_ptr->str_count_bak = string_stack.get_count();

	int i = 0, token_len;
	node_attribute_t *raw_token_ptr = (node_attribute_t*)this->interprete_need_ptr->mid_code_stack.get_current_ptr();
	while(len > 0) {
		token_len = get_token(str + rptr, &raw_token_ptr[i]);
		if(token_len < 0)
			return token_len;
		raw_token_ptr[i].pos += rptr;
		if(raw_token_ptr[i].node_type != TOKEN_NONEXIST) {
			if(raw_token_ptr[i].node_type == TOKEN_OPERATOR) {
				switch(raw_token_ptr[i].data) {
				case OPT_L_SMALL_BRACKET: case OPT_L_MID_BRACKET:
					bracket_stack[bracket_depth++] = raw_token_ptr[i].data;
					break;
				case OPT_R_SMALL_BRACKET:
					if(bracket_depth-- > 0 && bracket_stack[bracket_depth] == OPT_L_SMALL_BRACKET) {
						break;
					} else {
						error("Bracket unmatch.\n");
						return ERROR_BRACKET_UNMATCH;
					}
				case OPT_R_MID_BRACKET:
					if(bracket_depth-- > 0 && bracket_stack[bracket_depth] == OPT_L_MID_BRACKET) {
						break;
					} else {
						error("Bracket unmatch.\n");
						return ERROR_BRACKET_UNMATCH;
					}
				case OPT_EDGE:
					if(!bracket_depth) {
						this->interprete_need_ptr->row_pretreat_fifo.write(interprete_need_ptr->sentence_buf + rptr + token_len, len - token_len);
						interprete_need_ptr->sentence_buf[rptr + token_len] = 0;
					}
				}
			} else if(raw_token_ptr[i].node_type == TOKEN_OTHER) {
				if(raw_token_ptr[i].data == L_BIG_BRACKET || raw_token_ptr[i].data == R_BIG_BRACKET)
					if(!bracket_depth) {
						if(i) {
							this->interprete_need_ptr->row_pretreat_fifo.write(interprete_need_ptr->sentence_buf + rptr, len);
							interprete_need_ptr->sentence_buf[rptr] = 0;
							return i;
						} else {
							this->interprete_need_ptr->row_pretreat_fifo.write(interprete_need_ptr->sentence_buf + rptr + token_len, len - token_len);
							this->interprete_need_ptr->sentence_buf[rptr + token_len] = 0;
							return 1;
						}
					}
				break;
			}
		} else
			break;
		i++;
		rptr += token_len;
		len -= token_len;
	}

	if(bracket_depth) {
		error("Bracket unmatch.\n");
		return ERROR_BRACKET_UNMATCH;
	}
	//sentence_buf[wptr] = 0;
	return i;
}

int c_interpreter::post_treat(void)
{
	token_fifo.content_reset();
	if(exec_flag) {
		string_info *ptr;
		while(string_stack.get_count() > interprete_need_ptr->str_count_bak) {
			ptr = (string_info*)string_stack.pop();
			vfree(ptr->get_name());
		}
	}
	return ERROR_NO;
}

bool c_interpreter::gdb_check(void)
{
#if DEBUG_EN
	if(!kstrncmp(this->interprete_need_ptr->sentence_buf, "gdb", 3)) {
		if(this->interprete_need_ptr->sentence_buf[3] == ' ' || this->interprete_need_ptr->sentence_buf[3] == '\t') {
			gdb::parse(&this->interprete_need_ptr->sentence_buf[3]);
			return true;
		}
	}
#endif
	return false;
}

int c_interpreter::get_expression_type(char *str, varity_info *&ret_varity)
{
	int ret, len = kstrlen(str), count;
	mid_code *base_bak = (mid_code*)this->interprete_need_ptr->mid_code_stack.get_base_addr();
	int count_bak = this->interprete_need_ptr->mid_code_stack.get_count();
	//mid_code *pc_bak = this->pc;
	this->interprete_need_ptr->mid_code_stack.set_base(base_bak + count_bak);
	this->interprete_need_ptr->mid_code_stack.set_count(0);
	count = this->generate_token_list(str, len);
	generate_mid_code(this->token_node_ptr, count, false);
	node_attribute_t *node_ptr = (node_attribute_t*)this->interprete_need_ptr->sentence_analysis_data_struct.tree_root->value;
	ret = node_ptr->node_type;
	char *name = (char*)node_ptr->value.ptr_value;
	switch(ret) {
		case TOKEN_NAME:
			if(name[0] == LINK_VARITY_PREFIX || name[0] == TMP_VAIRTY_PREFIX) {
				ret_varity = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(node_ptr->value.ptr_value[1]);
			} else {
				ret_varity = (varity_info*)this->varity_declare->find(name);
			}
		default:
			break;
	} 
	this->interprete_need_ptr->mid_code_stack.set_base(base_bak);
	this->interprete_need_ptr->mid_code_stack.set_count(count_bak);
	//this->pc = pc_bak;
	return ret;
}

char* c_interpreter::code_complete_callback(char *tip_str, int no)
{
	int i = 0, j;
	int tiplen = kstrlen(tip_str);
	int count = c_interpreter::language_elment_space.g_varity_list.get_count();
	int member_flag = 0, member_pos, member_count;
	varity_info *varity_base = (varity_info*)c_interpreter::language_elment_space.g_varity_list.get_base_addr();
	for(j=tiplen-1; j>=-1; j--) {
		if(j >= 0 && (tip_str[j] == '.' || tip_str[j] == '>' && tip_str[j - 1] == '-')) {
			if(member_flag == 0) 
				if(tip_str[j] == '.') {
					tip_str[member_pos = j] = 0;
					member_count= 1;
					member_flag = 1;
				} else {
					tip_str[member_pos = j - 1] = 0;
					member_count= 2;
					member_flag = 1;
					j--;
				}
		} else if(j == -1 || !(tip_str[j] == '_' || kisalnum(tip_str[j]))) {
			if(member_flag)
				break;
			member_pos = j;
			member_count = -1;
			tiplen = kstrlen(&tip_str[j + 1]);
			break;
		}
	}
	if(!member_flag) {
		for(j=0; j<count; j++) {
			if(!kstrncmp(tip_str + member_pos + 1, varity_base[j].get_name(), tiplen)) {
				i++;
				if(i == no)
					return varity_base[j].get_name() + tiplen;
			}
		}
		count = c_interpreter::language_elment_space.function_list.get_count();
		function_info *function_base = (function_info*)c_interpreter::language_elment_space.function_list.get_base_addr();
		for(j=0; j<count; j++) {
			if(!kstrncmp(tip_str + member_pos + 1, function_base[j].get_name(), tiplen)) {
				i++;
				if(i == no)
					return function_base[j].get_name() + tiplen;
			}
		}
	} else {
		varity_info *varity_ptr = NULL;
		//tip_str[member_pos] = 0;
		tiplen = kstrlen(&tip_str[member_pos + member_count]);
		set_print_level(PRINT_LEVEL_MASKALL);
		int ret = myinterpreter.get_expression_type(tip_str + j + 1, varity_ptr);
		set_print_level(PRINT_LEVEL_DEFAULT);
		if(ret == TOKEN_NAME && varity_ptr) {
			PLATFORM_WORD *complex_ptr = varity_ptr->get_complex_ptr();
			struct_info *struct_info_ptr = NULL;
			count = varity_ptr->get_complex_arg_count();
			if(GET_COMPLEX_DATA(complex_ptr[count]) == STRUCT) {
				struct_info_ptr = (struct_info*)complex_ptr[count - 1];
				tip_str[member_pos] = '.';
			} else if(GET_COMPLEX_TYPE(complex_ptr[count]) == COMPLEX_PTR && GET_COMPLEX_DATA(complex_ptr[count - 1]) == STRUCT) {
				struct_info_ptr = (struct_info*)complex_ptr[count - 2];
				tip_str[member_pos] = '-';
			}
			if(struct_info_ptr) {
				count = struct_info_ptr->varity_stack_ptr->get_count();
				varity_base = (varity_info*)struct_info_ptr->varity_stack_ptr->get_base_addr();
				for(j=0; j<count; j++) {
					if(!kstrncmp(tip_str + member_pos + member_count, varity_base[j].get_name(), tiplen)) {
						i++;
						if(i == no)
							return varity_base[j].get_name() + tiplen;
					}
				}
			}
		}
	}
	return NULL;
}

int c_interpreter::run_interpreter(void)
{
	int ret;
	while(1) {
		int len;
		ret = ERROR_NO;
		len = this->interprete_need_ptr->row_pretreat_fifo.readline(interprete_need_ptr->sentence_buf);
		if(len > 0) {

		} else {
			if(this->tty_used == &stdio)
				kprintf(">>> ");
			len = tty_used->readline(interprete_need_ptr->sentence_buf, '`', c_interpreter::code_complete_callback);
			this->tty_used->line++;
			if(len == -1) {
				if(this->tty_used == &fileio) {
					this->tty_used->dispose();
				}
				break;
			}
		}
#if DEBUG_EN
		if(this->gdb_check()) {
			ret = gdb::exec(this);
			continue;
		}
#endif
		ret = preprocess(this->interprete_need_ptr->sentence_buf, len);
		if(ret == OK_PREPROCESS || ret < 0)
			continue;
		len = pre_treat(this->interprete_need_ptr->sentence_buf, len);
		if(len < 0)
			continue;
		ret = this->token_convert((node_attribute_t*)this->interprete_need_ptr->mid_code_stack.get_current_ptr(), len);
		//ret = this->generate_token_list(sentence_buf, len);
		if(ret < 0)
			if(this->tty_used == &stdio)
				continue;
			else
				break;
		ret = this->eval(this->token_node_ptr, ret);
		if(ret < 0)
			if(this->tty_used == &stdio)
				continue;
			else {
				error("line %d error.\n", this->tty_used->line);
				break;
			}
		if(this->tty_log) {
			this->tty_log->t_puts(this->interprete_need_ptr->sentence_buf);
			this->tty_log->t_putc('\n');
		}
		this->post_treat();
		if(ret == OK_FUNC_RETURN)
			break;
	}
	if(this->tty_log)
		this->tty_log->dispose();
	return ret;
}

void c_interpreter::set_tty(terminal* tty, terminal* ttylog)
{
	if(tty)
		this->tty_used = tty;
	this->tty_log = ttylog;
}

int c_interpreter::init(terminal* tty_used, int rtl_flag, int interprete_need, int stack_size)
{
	if(!c_interpreter::language_elment_space.init_done) {
		c_interpreter::language_elment_space.l_varity_list.init(sizeof(varity_info), MAX_L_VARITY_NODE);
		c_interpreter::language_elment_space.g_varity_list.init(sizeof(varity_info), MAX_G_VARITY_NODE);
		c_interpreter::language_elment_space.c_varity.init(&c_interpreter::language_elment_space.g_varity_list, &c_interpreter::language_elment_space.l_varity_list);
		c_interpreter::language_elment_space.function_list.init(sizeof(function_info), MAX_FUNCTION_NODE);
		c_interpreter::language_elment_space.c_function.init(&c_interpreter::language_elment_space.function_list);
		c_interpreter::language_elment_space.struct_list.init(sizeof(struct_info), MAX_STRUCT_NODE);
		c_interpreter::language_elment_space.c_struct.init(&c_interpreter::language_elment_space.struct_list);
		c_interpreter::language_elment_space.macro_list.init(sizeof(macro_info), DEFAULT_MACRO_NODE);
		c_interpreter::language_elment_space.c_macro.init(&c_interpreter::language_elment_space.macro_list);
		//c_interpreter::language_elment_space.init_done = 1;
		string_stack.init(sizeof(string_info), DEFAULT_STRING_NODE);
		name_stack.init(sizeof(string_info), DEFAULT_NAME_NODE);
		name_fifo.init(DEFAULT_NAME_LENGTH);
		for(int i=0; i<MAX_A_VARITY_NODE; i++) {
			link_varity_name[i][0] = LINK_VARITY_PREFIX;
			tmp_varity_name[i][0] = TMP_VAIRTY_PREFIX;
			tmp_varity_name[i][1] = link_varity_name[i][1] = i;
			tmp_varity_name[i][2] = link_varity_name[i][2] = 0;
		}
	}
	this->varity_declare = &c_interpreter::language_elment_space.c_varity;
	this->nonseq_info = &c_interpreter::language_elment_space.nonseq_info_s;
	this->function_declare = &c_interpreter::language_elment_space.c_function;
	this->struct_declare = &c_interpreter::language_elment_space.c_struct;
	this->macro_declare = &c_interpreter::language_elment_space.c_macro;
	if(interprete_need) {
		this->interprete_need_ptr = (interprete_need_t*)dmalloc(sizeof(interprete_need_s), "interprete need data space");
		this->interprete_need_ptr->sentence_analysis_data_struct.short_depth = 0;
		this->interprete_need_ptr->sentence_analysis_data_struct.sizeof_depth = 0;
		this->interprete_need_ptr->sentence_analysis_data_struct.label_count = 0;
		this->interprete_need_ptr->row_pretreat_fifo.set_base(this->interprete_need_ptr->pretreat_buf);
		this->interprete_need_ptr->row_pretreat_fifo.set_length(sizeof(this->interprete_need_ptr->pretreat_buf));
		this->interprete_need_ptr->row_pretreat_fifo.set_element_size(1);
		this->interprete_need_ptr->non_seq_code_fifo.set_base(this->interprete_need_ptr->non_seq_tmp_buf);
		this->interprete_need_ptr->non_seq_code_fifo.set_length(sizeof(this->interprete_need_ptr->non_seq_tmp_buf));
		this->interprete_need_ptr->non_seq_code_fifo.set_element_size(1);
		this->interprete_need_ptr->mid_code_stack.init(sizeof(mid_code), MAX_MID_CODE_COUNT);//
		this->interprete_need_ptr->mid_varity_stack.init(sizeof(varity_info), this->tmp_varity_stack, MAX_A_VARITY_NODE);//TODO: 设置node最大count
	}
	this->simulation_stack = (char*)dmalloc(stack_size, "interpreter stack space");
	this->simulation_stack_size = stack_size;
	//this->nonseq_info->nonseq_begin_stack_ptr = 0;
	this->real_time_link = rtl_flag;
	this->tty_used = tty_used;
	this->tty_used->init();
	this->call_func_info.function_depth = 0;
	this->call_func_info.para_offset = PLATFORM_WORD_LEN;
	this->varity_global_flag = VARITY_SCOPE_GLOBAL;
	///////////
	this->break_flag = 0;
	this->token_node_ptr = this->interprete_need_ptr->sentence_analysis_data_struct.node_attribute;
	this->stack_pointer = this->simulation_stack + PLATFORM_WORD_LEN;
	this->tmp_varity_stack_pointer = this->simulation_stack + this->simulation_stack_size;

	this->cur_mid_code_stack_ptr = &this->interprete_need_ptr->mid_code_stack;
	this->exec_flag = EXEC_FLAG_TRUE;
	if(!c_interpreter::language_elment_space.init_done) {
		varity_type_stack.init();
		for(int i=0; i<sizeof(basic_type_info)/sizeof(basic_type_info[0]); i++) {
			varity_type_stack.arg_count[i] = 2;
			varity_type_stack.type_info_addr[i] = basic_type_info[i];
		}
		varity_type_stack.arg_count[STRUCT] = 3;
		varity_type_stack.count = sizeof(basic_type_info) / sizeof(basic_type_info[0]);
		varity_type_stack.length = MAX_VARITY_TYPE_COUNT;
		handle_init();
		c_interpreter::cstdlib_func_count = 0;
		this->generate_compile_func();
		c_interpreter::language_elment_space.init_done = 1;
	}
	return ERROR_NO;
	///////////
}

int c_interpreter::dispose(void)
{
	string_stack.dispose();
	name_stack.dispose();
	name_fifo.dispose();
	varity_type_stack.dispose();
	for(int i=0; i<this->cstdlib_func_count; i++)
		((function_info*)this->function_declare->function_stack_ptr->visit_element_by_index(i))->arg_list->dispose();
	if(this->interprete_need_ptr) {
		this->interprete_need_ptr->mid_code_stack.dispose();
		vfree(interprete_need_ptr);
		interprete_need_ptr = 0;
	}
	c_interpreter::language_elment_space.struct_list.dispose();
	c_interpreter::language_elment_space.function_list.dispose();
	c_interpreter::language_elment_space.g_varity_list.dispose();
	c_interpreter::language_elment_space.l_varity_list.dispose();
	c_interpreter::language_elment_space.macro_list.dispose();
	vfree(this->simulation_stack);
	this->tty_used->dispose();
	c_interpreter::language_elment_space.init_done = 0;
	return ERROR_NO;
}

void nonseq_info_struct::reset(void)
{
	kmemset(this, 0, sizeof(nonseq_info_struct));
}

int c_interpreter::save_sentence(char* str, uint len)
{
	nonseq_info->row_info_node[nonseq_info->row_num].row_ptr = &this->interprete_need_ptr->non_seq_tmp_buf[interprete_need_ptr->non_seq_code_fifo.wptr];
	nonseq_info->row_info_node[nonseq_info->row_num].row_len = len;
	this->interprete_need_ptr->non_seq_code_fifo.write(str, len);
	this->interprete_need_ptr->non_seq_code_fifo.write("\0", 1);
	return 0;
}

#define GET_BRACE_FLAG(x)	(((x) & 128) >> 7)
#define GET_BRACE_DATA(x)	((x) & 127)
#define SET_BRACE(x,y) (((x) << 7) | (y))
int c_interpreter::struct_end(int struct_end_flag, bool &exec_flag_bak, register bool try_flag)
{
	int ret = 0;
	int depth_bak = this->nonseq_info->non_seq_struct_depth;
	while(this->nonseq_info->non_seq_struct_depth > 0 
		&& ((GET_BRACE_DATA(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1]) == nonseq_info->brace_depth + 1 && GET_BRACE_FLAG(nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1]))
		|| (GET_BRACE_DATA(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1]) == nonseq_info->brace_depth && !GET_BRACE_FLAG(nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1])))) {
		if(!try_flag) {
			this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1] = 0;
			this->varity_declare->local_varity_stack->dedeep();
		}
		if(nonseq_info->non_seq_struct_depth > 0 && nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] == NONSEQ_KEY_IF && !(struct_end_flag & 2)) {
			if(!try_flag)
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] = NONSEQ_KEY_WAIT_ELSE;
			--nonseq_info->non_seq_struct_depth;
			break;
		}
		--nonseq_info->non_seq_struct_depth;
		if(nonseq_info->non_seq_struct_depth >= 0 && !try_flag)
			nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = 0;
	}
	if(struct_end_flag & 1) {
		nonseq_info->row_info_node[nonseq_info->row_num].non_seq_info = 1;
		nonseq_info->row_info_node[nonseq_info->row_num].non_seq_depth = nonseq_info->non_seq_struct_depth + 1;
	}
	if(nonseq_info->non_seq_struct_depth == 0) {
		if(nonseq_info->non_seq_type_stack[0] == NONSEQ_KEY_WAIT_ELSE) {
			ret = OK_NONSEQ_INPUTING;
		} else if(nonseq_info->non_seq_type_stack[0] == NONSEQ_KEY_WAIT_WHILE) {
			ret = OK_NONSEQ_INPUTING;
		} else {
			if(!try_flag) {
				nonseq_info->non_seq_exec = 1;
				this->exec_flag = exec_flag_bak;
				this->varity_global_flag = exec_flag_bak;
			}
			ret = OK_NONSEQ_FINISH;
		}
	}
	if(try_flag)
		this->nonseq_info->non_seq_struct_depth = depth_bak;
	return ret;
}

int c_interpreter::non_seq_struct_analysis(node_attribute_t* node_ptr, uint count)
{
	static bool exec_flag_bak;
	int struct_end_flag = 0;
	int ret = ERROR_NO;
 	int current_brace_level = 0;
	if(count == 0 && nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] != NONSEQ_KEY_WAIT_ELSE)
		return ret;
#if DEBUG_EN
	nonseq_info->row_info_node[nonseq_info->row_num].row_code_ptr = (mid_code*)this->interprete_need_ptr->mid_code_stack.get_current_ptr();
#endif
	nonseq_info->last_non_seq_check_ret = nonseq_info->non_seq_check_ret;
	nonseq_info->non_seq_check_ret = node_ptr[0].node_type == TOKEN_KEYWORD_NONSEQ ? node_ptr[0].value.int_value : 0;
	current_brace_level = nonseq_info->brace_depth;
	if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_WAIT_ELSE) {
		if(nonseq_info->brace_depth == 0) {
			if(count != 0 && nonseq_info->non_seq_check_ret != NONSEQ_KEY_ELSE) {
				if(this->tty_used == &fileio || this->function_flag_set.function_flag) {
					goto if_end;
				}
				error("if is unmatch with else or blank\n");
				return ERROR_NONSEQ_GRAMMER;
			} else if(count == 0) {
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_IF;
				struct_end_flag = 2;
			} else { //nonseq_info->non_seq_check_ret == NONSEQ_KEY_ELSE
				nonseq_mid_gen_mid_code(node_ptr, count);//这句不会出错，不加返回值判断了
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 3;
			}
		} else {
			if(nonseq_info->non_seq_check_ret != NONSEQ_KEY_ELSE && count != 0) {//TODO:类型于函数中的if，对函数中的if处理可照此执行
				if_end:
				struct_end_flag = 2;
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_IF;
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth = nonseq_info->non_seq_struct_depth + 1;
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 1;
				nonseq_end_gen_mid_code(nonseq_info->row_num - 1, 0, 0);
				ret = struct_end(struct_end_flag, exec_flag_bak, 0);
				struct_end_flag = 0;
			} else if(nonseq_info->non_seq_check_ret == NONSEQ_KEY_ELSE) {
				nonseq_mid_gen_mid_code(node_ptr, count);
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 3;
			}
		}
	} else if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] == NONSEQ_KEY_WAIT_WHILE) {
		if(nonseq_info->non_seq_check_ret != NONSEQ_KEY_WHILE && count) {
			error("do is unmatch with while\n");
			nonseq_info->non_seq_check_ret = nonseq_info->last_non_seq_check_ret;
			return ERROR_NONSEQ_GRAMMER;
 		} else if(nonseq_info->non_seq_check_ret == NONSEQ_KEY_WHILE) {
			struct_end_flag = 1;
			goto struct_end_check;
		}
	}

	if(node_ptr[0].node_type == TOKEN_OTHER && node_ptr[0].data == L_BIG_BRACKET) {
		if(nonseq_info->non_seq_struct_depth) {
			nonseq_info->brace_depth++;
			if(nonseq_info->last_non_seq_check_ret) {
				this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1] = SET_BRACE(1, nonseq_info->brace_depth);
			} else {
				this->varity_declare->local_varity_stack->endeep();
			}
		} else if(nonseq_info->row_num == 0) {
			nonseq_info->brace_depth++;
			exec_flag_bak = this->exec_flag;
			if(exec_flag_bak == true)
				this->varity_global_flag = VARITY_SCOPE_LOCAL;
			this->exec_flag = EXEC_FLAG_FALSE;
			this->varity_declare->local_varity_stack->endeep();
			this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] = SET_BRACE(1, nonseq_info->brace_depth);
			nonseq_info->row_info_node[nonseq_info->row_num].non_seq_info = 0;
			nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_BRACE;
			nonseq_info->non_seq_struct_depth++;
			nonseq_info->row_info_node[nonseq_info->row_num].non_seq_depth = nonseq_info->non_seq_struct_depth;
			nonseq_info->row_info_node[nonseq_info->row_num].nonseq_type = nonseq_info->non_seq_check_ret;
			ret = OK_NONSEQ_DEFINE;
		}
	} else if(node_ptr[0].node_type == TOKEN_OTHER && node_ptr[0].data == R_BIG_BRACKET && count) {
		if(nonseq_info->brace_depth > 0) {
			if(nonseq_info->non_seq_struct_depth > 0 && GET_BRACE_DATA(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1]) == nonseq_info->brace_depth) {
				if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] != NONSEQ_KEY_DO) {
					if(!struct_end_flag)
						struct_end_flag = 1;
				} else
					nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] = NONSEQ_KEY_WAIT_WHILE;
			} else {
				this->varity_declare->local_varity_stack->dedeep();
			}
			nonseq_info->brace_depth--;
		} else {
			nonseq_info->non_seq_check_ret = nonseq_info->last_non_seq_check_ret;
			error("there is no { to match\n");
			return ERROR_NONSEQ_GRAMMER;
		}
	}

	if(nonseq_info->non_seq_check_ret) {
		if(nonseq_info->row_num == 0) {
			exec_flag_bak = this->exec_flag;
			if(exec_flag_bak == true) {
				this->varity_global_flag = VARITY_SCOPE_LOCAL;
			}
			this->exec_flag = EXEC_FLAG_FALSE;
		}
		this->varity_declare->local_varity_stack->endeep();
		ret = nonseq_start_gen_mid_code(node_ptr, count, nonseq_info->non_seq_check_ret);
		if(ret) {
			nonseq_info->non_seq_check_ret = nonseq_info->last_non_seq_check_ret;
			error("Nonseq struct error.\n");
			this->varity_declare->local_varity_stack->dedeep();
			if(nonseq_info->row_num == 0) {
				this->exec_flag = exec_flag_bak;
				this->varity_global_flag = exec_flag_bak;
			}
			return ret;
		}
		this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] = SET_BRACE(0, nonseq_info->brace_depth);
		nonseq_info->row_info_node[nonseq_info->row_num].non_seq_info = 0;
		if(nonseq_info->non_seq_check_ret == NONSEQ_KEY_ELSE) {
			nonseq_info->row_info_node[nonseq_info->row_num].non_seq_info = 2;
			if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] != NONSEQ_KEY_WAIT_ELSE) {//|| nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] != nonseq_info->brace_depth
				error("else is unmatch with if\n");
				return ERROR_NONSEQ_GRAMMER;
			}
		}
		nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = nonseq_info->non_seq_check_ret;
		nonseq_info->non_seq_struct_depth++;
		nonseq_info->row_info_node[nonseq_info->row_num].non_seq_depth = nonseq_info->non_seq_struct_depth;
		nonseq_info->row_info_node[nonseq_info->row_num].nonseq_type = nonseq_info->non_seq_check_ret;
		ret = OK_NONSEQ_DEFINE;
	}
	if(node_ptr[count - 1].data == OPT_EDGE && node_ptr[count - 1].node_type == TOKEN_OPERATOR) {
		if(nonseq_info->non_seq_struct_depth || ret == OK_NONSEQ_FINISH) { //1
			int eret = this->sentence_exec(node_ptr, count, true);
			if(eret) {
				nonseq_info->non_seq_check_ret = nonseq_info->last_non_seq_check_ret;//恢复上一句结果，否则重输正确语句无法结束非顺序结构
				return eret;
			}
		}
		if(nonseq_info->last_non_seq_check_ret && nonseq_info->non_seq_struct_depth) {// && nonseq_info->brace_depth == 0
			if(nonseq_info->last_non_seq_check_ret != NONSEQ_KEY_DO)
				struct_end_flag = 1;
			else
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] = NONSEQ_KEY_WAIT_WHILE;
		}
	}
struct_end_check:
	if(struct_end_flag) {
		ret = struct_end(struct_end_flag, exec_flag_bak, 1);
		nonseq_end_gen_mid_code(nonseq_info->row_num, node_ptr, count);
		ret = struct_end(struct_end_flag, exec_flag_bak, 0);
	}
	if(count && (nonseq_info->non_seq_struct_depth || nonseq_info->row_info_node[nonseq_info->row_num].non_seq_depth)) {// && (nonseq_info->non_seq_check_ret || nonseq_info->non_seq_struct_depth)) {
		this->save_sentence(this->interprete_need_ptr->sentence_buf, kstrlen(this->interprete_need_ptr->sentence_buf));
		nonseq_info->row_num++;
	}
	if(!ret) {
		if(count == 0 && !nonseq_info->row_num)
			ret = OK_NONSEQ_INPUTING;
		if(nonseq_info->non_seq_struct_depth)
			ret = OK_NONSEQ_INPUTING;
	}
	return ret;
}

int c_interpreter::generate_compile_func(void)
{
	static stack eval_stack;
	this->generate_arg_list("int,char*;", 2, eval_stack);
	this->function_declare->add_compile_func("eval", (void*)user_eval, &eval_stack, 0);
	///////////////////////////////////////////
	static stack memcpy_stack;
	this->generate_arg_list("void*,void*,void*,unsigned int;", 4, memcpy_stack);
	this->function_declare->add_compile_func("memcpy", (void*)kmemcpy, &memcpy_stack, 0);
	static stack memcmp_stack;
	this->generate_arg_list("int,void*,void*,unsigned int;", 4, memcmp_stack);
	this->function_declare->add_compile_func("memcmp", (void*)kmemcmp, &memcmp_stack, 0);
	static stack memset_stack;
	this->generate_arg_list("void*,void*,int,unsigned int;", 4, memset_stack);
	this->function_declare->add_compile_func("memset", (void*)kmemset, &memset_stack, 0);
	static stack printf_stack;
	this->generate_arg_list("int,char*;", 2, printf_stack);
	this->function_declare->add_compile_func("printf", (void*)kprintf, &printf_stack, 1);
	static stack sprintf_stack;
	this->generate_arg_list("int,char*,char*;", 3, sprintf_stack);
	this->function_declare->add_compile_func("sprintf", (void*)ksprintf, &sprintf_stack, 1);
	static stack strcmp_stack;
	this->generate_arg_list("int,char*,char*;", 3, strcmp_stack);
	this->function_declare->add_compile_func("strcmp", (void*)kstrcmp, &strcmp_stack, 0);
	static stack irqreg_stack;
	this->generate_arg_list("void,int,void*,void*;", 4, irqreg_stack);
	this->function_declare->add_compile_func("irq_reg", (void*)irq_reg, &irqreg_stack, 0);
	static stack refscript_stack;
	this->generate_arg_list("int,char*;", 2, refscript_stack);
	this->function_declare->add_compile_func("ref", (void*)refscript, &refscript_stack, 0);
	this->cstdlib_func_count = this->function_declare->function_stack_ptr->get_count();
	return ERROR_NO;
}

int c_interpreter::generate_arg_list(const char *str, int count, stack &arg_list_ptr)//没有容错，不开放给终端输入，仅用于链接标准库函数，仅能使用1级指针
{
	int len = kstrlen(str);
	void *arg_stack = dmalloc(sizeof(varity_info) * count, "arg stack");
	arg_list_ptr.init(sizeof(varity_info), arg_stack, count);
	varity_info *varity_ptr = (varity_info*)arg_list_ptr.get_base_addr();
	node_attribute_t node;
	int token_len;
	int type;
	char ptr_flag = 0;
	while(len > 0) {
		token_len = get_token((char*)str, &node);
		if(node.node_type == TOKEN_KEYWORD_TYPE) {
			type = node.value.int_value;
		} else if(node.node_type == TOKEN_OPERATOR ) {
			if(node.data == OPT_MUL) {
				ptr_flag = 1;
			} else {
				varity_ptr->init_varity(0, 4, 1 + ptr_flag, basic_type_info[type]);
				varity_ptr++;
				arg_list_ptr.push();
				ptr_flag = 0;
			}
		}
		str += token_len;
		len -= token_len;
	}
	return ERROR_NO;
}

#define ARG_RETURN(x) this->function_flag_set.function_flag = 0; \
	this->function_flag_set.function_begin_flag = 0; \
	destroy_varity_stack(arg_stack); \
	vfree(arg_stack->get_base_addr()); \
	vfree(arg_stack); \
	this->varity_declare->destroy_local_varity(); \
	this->cur_mid_code_stack_ptr = &this->mid_code_stack; \
	this->exec_flag = EXEC_FLAG_TRUE; \
	this->varity_global_flag = VARITY_SCOPE_GLOBAL; \
	return x
int c_interpreter::function_analysis(node_attribute_t* node_ptr, int count)
{//TODO:非调试使能删除函数源代码
	int ret_function_define;
	if(this->function_flag_set.function_flag) {
		if(!count)
			return OK_FUNC_INPUTING;
		function_info *current_function_ptr = this->function_declare->get_current_node();
#if DEBUG_EN
		current_function_ptr->row_code_ptr[current_function_ptr->row_line] = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
#endif
		this->function_declare->save_sentence(this->interprete_need_ptr->sentence_buf, kstrlen(this->interprete_need_ptr->sentence_buf));
		if(this->function_flag_set.function_begin_flag) {
			if(node_ptr[0].node_type != TOKEN_OTHER || node_ptr[0].data != L_BIG_BRACKET) {
				error("Please enter {\n");
				return ERROR_FUNC_ERROR;
			}
			this->function_flag_set.function_begin_flag = 0;
			node_ptr[0].data = L_BIG_BRACKET_F;
		}
		if(node_ptr[0].node_type == TOKEN_OTHER && (node_ptr[0].data == L_BIG_BRACKET || node_ptr[0].data == L_BIG_BRACKET_F)) {
			this->function_flag_set.brace_depth++;
		} else if(node_ptr[0].node_type == TOKEN_OTHER && node_ptr[0].data == R_BIG_BRACKET) {
			this->function_flag_set.brace_depth--;
			if(!this->function_flag_set.brace_depth) {
				node_ptr[0].data = R_BIG_BRACKET_F;
				mid_code *mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr(), *code_end_ptr = mid_code_ptr;
				mid_code_ptr->ret_operator = CTL_BXLR;
				mid_code_ptr->opdb_addr = ((varity_info*)current_function_ptr->arg_list->visit_element_by_index(0))->get_type();//return value type.
				this->cur_mid_code_stack_ptr->push();
				while(--mid_code_ptr >= (mid_code*)current_function_ptr->mid_code_stack.get_base_addr()) {
					if(mid_code_ptr->ret_operator == CTL_RETURN)
						mid_code_ptr->opda_addr = code_end_ptr - mid_code_ptr;
					else if(mid_code_ptr->ret_operator == CTL_GOTO) {
						int i;
						for(i=0; i<this->interprete_need_ptr->sentence_analysis_data_struct.label_count; i++) {
							if(!kstrcmp(this->interprete_need_ptr->sentence_analysis_data_struct.label_name[i], (char*)&mid_code_ptr->ret_addr)) {
								mid_code_ptr->opda_addr = (mid_code*)this->interprete_need_ptr->sentence_analysis_data_struct.label_addr[i] - mid_code_ptr;
								break;
							}
						}
						if(i == this->interprete_need_ptr->sentence_analysis_data_struct.label_count) {
							error("No label called \"%s\"\n", (char*)&mid_code_ptr->ret_addr);
							current_function_ptr->reset();
							vfree(current_function_ptr->arg_list->get_base_addr());
							vfree(current_function_ptr->arg_list);
							this->varity_declare->destroy_local_varity();
							return ERROR_GOTO_LABEL;
						}
					}
				}
#if DEBUG_EN
				current_function_ptr->copy_local_varity_stack(this->varity_declare->local_varity_stack);
#endif
				this->function_flag_set.function_flag = 0;
				this->cur_mid_code_stack_ptr = &this->interprete_need_ptr->mid_code_stack;
				current_function_ptr->stack_frame_size = this->varity_declare->local_varity_stack->offset;
				current_function_ptr->size_adapt();
				this->exec_flag = EXEC_FLAG_TRUE;
				this->varity_global_flag = VARITY_SCOPE_GLOBAL;
				this->varity_declare->destroy_local_varity();
				this->interprete_need_ptr->sentence_analysis_data_struct.label_count = 0;
				if(this->real_time_link)
					this->ulink(&current_function_ptr->mid_code_stack, LINK_ADDR);
				//this->ulink(&current_function_ptr->mid_code_stack);
				return OK_FUNC_FINISH;
			}
		}
		return OK_FUNC_INPUTING;
	}
	int basic_count = count;
	struct_info *struct_info_ptr;
	ret_function_define = basic_type_check(node_ptr, basic_count, struct_info_ptr);
	if(ret_function_define >= 0) {
		char function_name[32];
		int v_count = count - basic_count;
		int func_flag = 0;
		PLATFORM_WORD *complex_info;
		int complex_count = get_varity_type(node_ptr + basic_count, v_count, function_name, ret_function_define, struct_info_ptr, complex_info);
		if(complex_count > 0 && GET_COMPLEX_TYPE(complex_info[complex_count]) == COMPLEX_ARG) {
			stack *arg_stack_ptr = (stack*)complex_info[complex_count - 1];
			if(complex_count != basic_count + v_count) {
				if(complex_count + 1 == basic_count + v_count && node_ptr[v_count].node_type == TOKEN_OPERATOR && node_ptr[v_count].data == OPT_EDGE)
					func_flag = FUNC_FLAG_PROTOTYPE;
				else {
					clear_arglist(arg_stack_ptr);
					vfree(complex_info);
					error("Extra symbol after function define.\n");
					return ERROR_FUNC_ERROR;
				}
			}
			if(!this->exec_flag) {
				clear_arglist(arg_stack_ptr);
				vfree(complex_info);
				error("Cannot define function here.\n");
				return ERROR_FUNC_DEF_POS;
			}
			uint arg_count = arg_stack_ptr->get_count();
			varity_info *arg_ptr = (varity_info*)arg_stack_ptr->get_base_addr();
			varity_info *all_arg_ptr = (varity_info*)dmalloc(sizeof(varity_info) * (arg_count + 1), "function's all args");
			kmemcpy(all_arg_ptr + 1, arg_ptr, sizeof(varity_info) * (arg_count));

			for(int n=0; n<arg_count; n++) {
				int size = get_varity_size(0, arg_ptr[n].get_complex_ptr(), arg_ptr[n].get_complex_arg_count());
				size = make_align(size, 4);
				this->varity_declare->declare(VARITY_SCOPE_LOCAL, arg_ptr[n].get_name(), size, arg_ptr[n].get_complex_arg_count(), arg_ptr[n].get_complex_ptr());
			}
			vfree(arg_ptr);
			arg_stack_ptr->set_base(all_arg_ptr);
			arg_stack_ptr->set_count(arg_count + 1);
			all_arg_ptr->config_complex_info(complex_count - 2, complex_info);
			inc_varity_ref(all_arg_ptr);
			ret_function_define = this->function_declare->declare(function_name, arg_stack_ptr, func_flag);
			if(ret_function_define) {
				clear_arglist(arg_stack_ptr);
				vfree(complex_info);
				destroy_varity_stack(this->varity_declare->local_varity_stack);
				//TODO:函数重复声明后变量inc/dec不平衡
				return ret_function_define;
			}
			if(!(func_flag & FUNC_FLAG_PROTOTYPE)) {
				this->function_flag_set.function_flag = 1;
				this->function_flag_set.function_begin_flag = 1;
				this->varity_global_flag = VARITY_SCOPE_LOCAL;
				this->cur_mid_code_stack_ptr = &this->function_declare->get_current_node()->mid_code_stack;
				this->exec_flag = false;
				this->function_declare->save_sentence(this->interprete_need_ptr->sentence_buf, kstrlen(this->interprete_need_ptr->sentence_buf));
			}
			return OK_FUNC_DEFINE;
		} else if(complex_count < 0)
			return complex_count;
	}
	return OK_FUNC_NOFUNC;
}
#undef ARG_RETURN

int c_interpreter::label_analysis(node_attribute_t *node_ptr, int count)
{
	if(count != 2)
		return ERROR_NO;
	if(node_ptr[0].node_type == TOKEN_NAME) {
		if(node_ptr[1].node_type == TOKEN_OPERATOR && node_ptr[1].data == OPT_TERNARY_C) {
			if(this->cur_mid_code_stack_ptr == &this->interprete_need_ptr->mid_code_stack) {
				error("Label & \"goto\" cannot be used in main function.\n");
				return ERROR_GOTO_POSITION;
			}
			if(this->interprete_need_ptr->sentence_analysis_data_struct.label_count >= MAX_LABEL_COUNT) {
				error("Label count reach max.\n");
				return ERROR_GOTO_COUNT_MAX;
			}
			this->interprete_need_ptr->sentence_analysis_data_struct.label_addr[this->interprete_need_ptr->sentence_analysis_data_struct.label_count] = this->cur_mid_code_stack_ptr->get_current_ptr();
			kstrcpy(this->interprete_need_ptr->sentence_analysis_data_struct.label_name[this->interprete_need_ptr->sentence_analysis_data_struct.label_count++], node_ptr[0].value.ptr_value);
			return OK_LABEL_DEFINE;
		}
	}
	return ERROR_NO;
}

int c_interpreter::post_order_expression(node_attribute_t *node_ptr, int count, list_stack &expression_final_stack)
{
	sentence_analysis_data_struct_t *analysis_data_struct_ptr = &this->interprete_need_ptr->sentence_analysis_data_struct;
	node_attribute_t *node_attribute, *stack_top_node_ptr;
	list_stack expression_tmp_stack;
	this->interprete_need_ptr->sentence_analysis_data_struct.last_token.node_type = TOKEN_OPERATOR;
	this->interprete_need_ptr->sentence_analysis_data_struct.last_token.data = OPT_EDGE;
	for(int i=0; i<count;i++) {
		node_attribute = &node_ptr[i];
		analysis_data_struct_ptr->node_struct[i].value = node_attribute;
		if(node_attribute->node_type == TOKEN_OPERATOR) {
			while(1) {
				stack_top_node_ptr = (node_attribute_t*)expression_tmp_stack.get_lastest_element()->value;
				if(!expression_tmp_stack.get_count() 
					|| node_attribute->value_type < stack_top_node_ptr->value_type 
					|| node_attribute->data == OPT_L_SMALL_BRACKET 
					|| stack_top_node_ptr->data == OPT_L_SMALL_BRACKET
					|| (node_attribute->value_type == stack_top_node_ptr->value_type && (node_attribute->value_type == 2 || node_attribute->value_type == 14))) {
					if(node_attribute->data == OPT_R_SMALL_BRACKET) {
						expression_tmp_stack.pop();
						break;
					} else {
						if(node_attribute->data == OPT_PLUS_PLUS || node_attribute->data == OPT_MINUS_MINUS) {
							node_attribute_t *final_stack_top_ptr = (node_attribute_t*)expression_final_stack.get_lastest_element()->value;
							if(!expression_final_stack.get_count() || this->interprete_need_ptr->sentence_analysis_data_struct.last_token.node_type == TOKEN_OPERATOR) {
								if(node_attribute->data == OPT_PLUS_PLUS)
									node_attribute->data = OPT_L_PLUS_PLUS;
								else
									node_attribute->data = OPT_L_MINUS_MINUS;
							} else {
								if(node_attribute->data == OPT_PLUS_PLUS)
									node_attribute->data = OPT_R_PLUS_PLUS;
								else
									node_attribute->data = OPT_R_MINUS_MINUS;
							}
						} else if(node_attribute->data == OPT_COMMA) {
							int element_count = expression_tmp_stack.get_count();
							node *cnode_ptr = expression_tmp_stack.get_lastest_element();
							for(int n=1; n<element_count; n++, cnode_ptr = cnode_ptr->left) {
								if(((node_attribute_t*)cnode_ptr->value)->node_type == TOKEN_OPERATOR && ((node_attribute_t*)cnode_ptr->value)->data == OPT_L_SMALL_BRACKET) {
									if(((node_attribute_t*)cnode_ptr->left->value)->node_type == TOKEN_OPERATOR && ((node_attribute_t*)cnode_ptr->left->value)->data == OPT_CALL_FUNC) {
										node_attribute->data = OPT_FUNC_COMMA;
									}
									break;
								}
							}
						}
						expression_tmp_stack.push(&analysis_data_struct_ptr->node_struct[i]);
						break;
					}
				} else {//Notion: priority of right parenthese should be lowwest.
					expression_final_stack.push(expression_tmp_stack.pop());
				}
			}
		} else if(node_attribute->node_type == TOKEN_NAME || node_attribute->node_type == TOKEN_CONST_VALUE || node_attribute->node_type == TOKEN_STRING) {
			expression_final_stack.push(&analysis_data_struct_ptr->node_struct[i]);
		}
		this->interprete_need_ptr->sentence_analysis_data_struct.last_token = *node_attribute;
	}
	while(expression_tmp_stack.get_count()) {
		expression_final_stack.push(expression_tmp_stack.pop());
	}
	return ERROR_NO;
}

int c_interpreter::ctl_analysis(node_attribute_t *node_ptr, int count)
{
	mid_code *mid_code_ptr;
	if(node_ptr[0].node_type == TOKEN_KEYWORD_CTL) {
		if(node_ptr[0].value.int_value == CTL_KEY_RETURN) {
			int ret;
			if(this->cur_mid_code_stack_ptr == &this->interprete_need_ptr->mid_code_stack)
				return OK_FUNC_RETURN;
			ret = this->generate_mid_code(node_ptr + 1, count - 1, false);
			if(ret)
				return ret;
			mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
			int func_ret_type = ((varity_info*)this->function_declare->get_current_node()->arg_list->visit_element_by_index(0))->get_type();
			if(((node_attribute_t*)root->value)->node_type == TOKEN_NAME && ((node_attribute_t*)root->value)->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {//$0已填入
				varity_info *ret_varity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(0);//because ((node_attribute_t*)root->value)->value.ptr_value[1]==0
				if(ret_varity_ptr->get_type() != func_ret_type) {
					mid_code_ptr->ret_varity_type = mid_code_ptr->opda_varity_type = func_ret_type;
					mid_code_ptr->opdb_varity_type = ret_varity_ptr->get_type();
					mid_code_ptr->opda_operand_type = mid_code_ptr->opdb_operand_type = mid_code_ptr->ret_operand_type = OPERAND_T_VARITY;
					mid_code_ptr->ret_addr = mid_code_ptr->opda_addr = mid_code_ptr->opdb_addr = 0;
					mid_code_ptr->ret_operator = OPT_ASSIGN;
					this->cur_mid_code_stack_ptr->push();
				}
			} else {
				mid_code_ptr->opda_operand_type = mid_code_ptr->ret_operand_type = OPERAND_T_VARITY;
				mid_code_ptr->ret_addr = mid_code_ptr->opda_addr = 0;
				mid_code_ptr->ret_varity_type = mid_code_ptr->opda_varity_type = func_ret_type;
				mid_code_ptr->ret_operator = OPT_ASSIGN;
				mid_code_ptr->opdb_varity_type = (mid_code_ptr - 1)->opdb_varity_type;
				mid_code_ptr->opdb_operand_type = (mid_code_ptr - 1)->ret_operand_type;
				mid_code_ptr->opdb_addr = (mid_code_ptr - 1)->ret_addr;
				this->cur_mid_code_stack_ptr->push();
			}
			mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
			mid_code_ptr->ret_operator = CTL_RETURN;
			this->cur_mid_code_stack_ptr->push();
			return ERROR_NO;
		} else if(node_ptr[0].value.int_value == CTL_KEY_BREAK) {
			if(this->nonseq_info->row_num > 0) {
				int read_seq_depth = this->nonseq_info->non_seq_struct_depth > this->nonseq_info->row_info_node[this->nonseq_info->row_num].non_seq_depth ? this->nonseq_info->non_seq_struct_depth : this->nonseq_info->row_info_node[this->nonseq_info->row_num].non_seq_depth;
				for(int i=read_seq_depth; i>0; i--) {
					if(this->nonseq_info->non_seq_type_stack[i - 1] == NONSEQ_KEY_FOR || this->nonseq_info->non_seq_type_stack[i - 1] == NONSEQ_KEY_WHILE || this->nonseq_info->non_seq_type_stack[i - 1] == NONSEQ_KEY_DO) {
						mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
						mid_code_ptr->ret_operator = CTL_BREAK;
						this->cur_mid_code_stack_ptr->push();
						return ERROR_NO;
					}
				}
			}
			error("no nonseq struct for using break.\n");
			return ERROR_NONSEQ_CTL;
		} else if(node_ptr[0].value.int_value == CTL_KEY_CONTINUE) {
			if(this->nonseq_info->row_num > 0) {
				int read_seq_depth = this->nonseq_info->non_seq_struct_depth > this->nonseq_info->row_info_node[this->nonseq_info->row_num].non_seq_depth ? this->nonseq_info->non_seq_struct_depth : this->nonseq_info->row_info_node[this->nonseq_info->row_num].non_seq_depth;
				for(int i=read_seq_depth; i>0; i--) {
					if(this->nonseq_info->non_seq_type_stack[i - 1] == NONSEQ_KEY_FOR || this->nonseq_info->non_seq_type_stack[i - 1] == NONSEQ_KEY_WHILE || this->nonseq_info->non_seq_type_stack[i - 1] == NONSEQ_KEY_DO) {
						mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
						mid_code_ptr->ret_operator = CTL_CONTINUE;
						this->cur_mid_code_stack_ptr->push();
						return ERROR_NO;
					}
				}
			}
			error("no nonseq struct for using continue.\n");
			return ERROR_NONSEQ_CTL;
		} else if(node_ptr[0].value.int_value == CTL_KEY_GOTO) {
			if(node_ptr[1].node_type != TOKEN_NAME) {
				error("There must be a label after goto.\n");
				return ERROR_GOTO_LABEL;
			}
			mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
			mid_code_ptr->ret_operator = CTL_GOTO;
			kstrcpy((char*)&mid_code_ptr->ret_addr, node_ptr[1].value.ptr_value);
			this->cur_mid_code_stack_ptr->push();
			return ERROR_NO;
		}		
	}
	return OK_CTL_NOT_FOUND;
}

int c_interpreter::generate_mid_code(node_attribute_t *node_ptr, int count, bool need_semicolon)//TODO:所有uint len统统改int，否则传个-1进来
{
	if(count == 0 || node_ptr->node_type == TOKEN_OTHER)return ERROR_NO;
	int ret = ERROR_NO;
	ret = this->ctl_analysis(node_ptr, count);
	if(ret != OK_CTL_NOT_FOUND)
		return ret;
	list_stack expression_stack;
	ret = this->post_order_expression(node_ptr, count, expression_stack);
	if(ret)
		goto gcode_exit;
	if(need_semicolon) {
		node *last_token = expression_stack.pop();
		if(((node_attribute_t*)last_token->value)->node_type != TOKEN_OPERATOR || ((node_attribute_t*)last_token->value)->data != OPT_EDGE) {
			error("Missing ;\n");
			ret = ERROR_SEMICOLON;
			goto gcode_exit;
		}
	}
	//后序表达式构造完成，下面构造二叉树
	//while(expression_stack.get_count()) {
	//	node_attribute_t *tmp = (node_attribute_t*)expression_stack.pop()->value;
	//	debug("%d ",tmp->node_type);
	//	if(tmp->node_type == TOKEN_NAME)
	//		debug("%s\n",tmp->value.ptr_value);
	//	else if(tmp->node_type == TOKEN_STRING)
	//		debug("%s\n", tmp->value.ptr_value);
	//	else if(tmp->node_type == TOKEN_CONST_VALUE)
	//		debug("%d\n", tmp->value.int_value);
	//	else
	//		debug("%d %d\n",tmp->data,tmp->value_type);
	//}
	root = expression_stack.pop();
	this->interprete_need_ptr->sentence_analysis_data_struct.tree_root = root;
	if(!root) {
		warning("No token found.\n");
		return 0;//TODO:找个合适的返回值
	}

	if(expression_stack.get_count() == 0) {
		ret = generate_expression_value(this->cur_mid_code_stack_ptr, (node_attribute_t*)root->value);
		if(ret)
			goto gcode_exit;
	} else {
		root->link_reset();
		ret = list_to_tree(root, &expression_stack);//二叉树完成
		if(ret) {
			expression_stack.reset();
			goto gcode_exit;
		}
		if(expression_stack.get_count()) {
			expression_stack.reset();
			error("Exist extra token.\n");
			ret = ERROR_OPERAND_SURPLUS;
			goto gcode_exit;
		}
		//root->middle_visit();
		int current_code_count = this->cur_mid_code_stack_ptr->get_count();
		ret = this->tree_to_code(root, this->cur_mid_code_stack_ptr);//构造中间代码
		if(ret) {
			while(this->interprete_need_ptr->mid_varity_stack.get_count()) {
				varity_info *tmp_varity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.pop();
				if(tmp_varity_ptr->get_complex_arg_count())
					dec_varity_ref(tmp_varity_ptr, true);
			}
			this->cur_mid_code_stack_ptr->del_element_to(current_code_count);
			this->call_func_info.function_depth = 0;
			this->call_func_info.para_offset = PLATFORM_WORD_LEN;
			goto gcode_exit;
		}
		if(this->interprete_need_ptr->sentence_analysis_data_struct.short_depth) {
			error("? && : unmatch.\n");
			this->interprete_need_ptr->sentence_analysis_data_struct.short_depth = 0;
			while(this->interprete_need_ptr->mid_varity_stack.get_count()) {
				varity_info *tmp_varity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.pop();
				dec_varity_ref(tmp_varity_ptr, true);
			}
			this->cur_mid_code_stack_ptr->del_element_to(current_code_count);
			this->call_func_info.function_depth = 0;
			this->call_func_info.para_offset = PLATFORM_WORD_LEN;
			ret = ERROR_TERNARY_UNMATCH;
			goto gcode_exit;
		}
	}
	if(this->interprete_need_ptr->mid_varity_stack.get_count()) {
		dec_varity_ref((varity_info*)this->interprete_need_ptr->mid_varity_stack.get_base_addr(), true);
		this->interprete_need_ptr->mid_varity_stack.pop();
	}
	if(((node_attribute_t*)root->value)->node_type == TOKEN_NAME && ((node_attribute_t*)root->value)->value.ptr_value[0] == LINK_VARITY_PREFIX) {
		generate_expression_value(this->cur_mid_code_stack_ptr, (node_attribute_t*)root->value);
		return ERROR_NO;
	}
	debug("generate code.\n");
gcode_exit:
	if(ret) {
		if(this->function_flag_set.function_flag)
			this->function_declare->destroy_sentence();
	}
	return ret;
}

int c_interpreter::generate_expression_value(stack *code_stack_ptr, node_attribute_t *node_attribute)//TODO:加参数，表示生成的赋值到哪个变量里。
{
	mid_code *instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
	varity_info *new_varity_ptr;
	if(node_attribute->node_type == TOKEN_NAME && (node_attribute->value.ptr_value[0] == TMP_VAIRTY_PREFIX || node_attribute->value.ptr_value[0] == LINK_VARITY_PREFIX)) {
		instruction_ptr->opda_addr = 8 * node_attribute->value.ptr_value[1];
	} else {
		instruction_ptr->opda_addr = this->interprete_need_ptr->mid_varity_stack.get_count() * 8;
		new_varity_ptr = (varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(this->interprete_need_ptr->mid_varity_stack.get_count());
		this->interprete_need_ptr->mid_varity_stack.push();
	}
	instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
	instruction_ptr->ret_operator = OPT_ASSIGN;
	instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
	if(node_attribute->node_type == TOKEN_CONST_VALUE) {
		instruction_ptr->opdb_operand_type = OPERAND_CONST;
		instruction_ptr->opdb_varity_type = node_attribute->value_type;
		kmemcpy(&instruction_ptr->opdb_addr, &node_attribute->value, 8);
		new_varity_ptr->set_type(node_attribute->value_type);
		inc_varity_ref(new_varity_ptr);
	} else if(node_attribute->node_type == TOKEN_NAME) {
		if(node_attribute->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {
			instruction_ptr->opdb_operand_type = OPERAND_T_VARITY;
			instruction_ptr->opdb_addr = 8 * node_attribute->value.ptr_value[1];
			instruction_ptr->opdb_varity_type = ((varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]))->get_type();
		} else if(node_attribute->value.ptr_value[0] == LINK_VARITY_PREFIX) {
			instruction_ptr->opdb_operand_type = OPERAND_LINK_VARITY;
			instruction_ptr->opdb_addr = (PLATFORM_WORD)((varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]))->get_content_ptr();
			instruction_ptr->opdb_varity_type = ((varity_info*)this->interprete_need_ptr->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]))->get_type();
		} else {
			varity_info *varity_ptr;
			int varity_scope;
			varity_ptr = this->varity_declare->vfind(node_attribute->value.ptr_value, varity_scope);
			if(!varity_ptr) {
				this->interprete_need_ptr->mid_varity_stack.pop();
				tip_wrong(node_attribute->pos);
				error("Varity not exist.\n");
				return ERROR_VARITY_NONEXIST;
			}
			if(varity_scope == VARITY_SCOPE_GLOBAL) {
				instruction_ptr->opdb_operand_type = OPERAND_G_VARITY;
				instruction_ptr->opdb_addr = (PLATFORM_WORD)varity_ptr->get_name();
			} else {
				instruction_ptr->opdb_operand_type = OPERAND_L_VARITY;
				instruction_ptr->opdb_addr = (PLATFORM_WORD)varity_ptr->get_content_ptr();
			}
			instruction_ptr->opdb_varity_type = varity_ptr->get_type();
			//new_varity_ptr->set_type(instruction_ptr->opdb_varity_type);
			new_varity_ptr->config_complex_info(varity_ptr->get_complex_arg_count(), varity_ptr->get_complex_ptr());
			inc_varity_ref(new_varity_ptr);
		}
	} else {
		error("No right name for expression.\n");
		return ERROR_OPERAND_LACKED;
	}
	instruction_ptr->opda_varity_type = instruction_ptr->opdb_varity_type;
	instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type;
	instruction_ptr->ret_addr = instruction_ptr->opda_addr;//TODO：需要吗？核对一下handle
	code_stack_ptr->push();
	return ERROR_NO;
}
extern int opt_time;
ITCM_TEXT int c_interpreter::exec_mid_code(mid_code *pc, uint count)
{
	int ret;
	mid_code *end_ptr;
	this->pc = pc;
	//int tick1, tick2 = HWREG(0x2040018), total1 = 0, total2 = 0;
	opt_time = 0;
	if(this->interprete_need_ptr && pc == this->interprete_need_ptr->mid_code_stack.get_base_addr()) {
		end_ptr = pc + count;
		while(this->pc != end_ptr) {
			//tick1 = HWREG(0x2040018);
			//total1 += tick2 - tick1;

			ret = call_opt_handle(this);
			//tick2 = HWREG(0x2040018);
			//total2 += tick1 - tick2;
			if(ret) {
				this->print_call_stack();
				this->stack_pointer = this->simulation_stack + PLATFORM_WORD_LEN;
				this->tmp_varity_stack_pointer = this->simulation_stack + this->simulation_stack_size;
				this->call_func_info.function_depth = 0;
				this->call_func_info.para_offset = PLATFORM_WORD_LEN;
				return ret;
			}
			this->pc++;
		}
	} else {
		end_ptr = pc + count - 1;
		while(this->pc != end_ptr || this->pc->ret_operator != CTL_BXLR) {
			//tick1 = HWREG(0x2040018);
			//total1 += tick2 - tick1;

			ret = call_opt_handle(this);
			//tick2 = HWREG(0x2040018);
			//total2 += tick1 - tick2;
			if(ret) {
				this->print_call_stack();
				this->stack_pointer = this->simulation_stack + PLATFORM_WORD_LEN;
				this->tmp_varity_stack_pointer = this->simulation_stack + this->simulation_stack_size;
				this->call_func_info.function_depth = 0;
				this->call_func_info.para_offset = PLATFORM_WORD_LEN;
				return ret;
			}
			this->pc++;
		}
		this->stack_pointer = this->simulation_stack + PLATFORM_WORD_LEN;
		this->tmp_varity_stack_pointer = this->simulation_stack + this->simulation_stack_size;
		this->call_func_info.function_depth = 0;
		this->call_func_info.para_offset = PLATFORM_WORD_LEN;
	}
	//kprintf("t1=%d,t2=%d,ot=%d\n", total1, total2, opt_time);
	return ERROR_NO;
}

int c_interpreter::eval(node_attribute_t* node_ptr, int count)
{
	int ret1 = 0, ret2 = 0;
	ret1 = this->label_analysis(node_ptr, count);
	if(ret1 != ERROR_NO)
		return ret1;
	ret1 = struct_analysis(node_ptr, count);
	if(ret1 != OK_STRUCT_NOSTRUCT)
		return ERROR_NO;
	ret1 = function_analysis(node_ptr, count);
	if(ret1 < 0 || ret1 == OK_FUNC_FINISH)
		return ret1;
	ret2 = non_seq_struct_analysis(node_ptr, count);
	if(ret2 == OK_NONSEQ_FINISH || ret2 == OK_NONSEQ_INPUTING || ret2 == OK_NONSEQ_DEFINE) {
		if(this->cur_mid_code_stack_ptr == &this->interprete_need_ptr->mid_code_stack && ret2 == OK_NONSEQ_FINISH) {
			this->varity_global_flag = VARITY_SCOPE_GLOBAL;
			//this->nonseq_info->stack_frame_size = this->varity_declare->local_varity_stack->offset;
			this->varity_declare->destroy_local_varity_cur_depth();
		}
	} else if(ret2 < 0)
		return ret2;
	
	if(nonseq_info->non_seq_exec) {
		debug("exec non seq struct\n");
		nonseq_info->non_seq_exec = 0;
		if(this->exec_flag) {
			this->ulink(this->cur_mid_code_stack_ptr, LINK_ADDR);
			this->print_code((mid_code*)this->cur_mid_code_stack_ptr->get_base_addr(), this->cur_mid_code_stack_ptr->get_count(), INTERPRETER_DEBUG);
			this->exec_mid_code((mid_code*)this->cur_mid_code_stack_ptr->get_base_addr(), this->cur_mid_code_stack_ptr->get_count());
			this->cur_mid_code_stack_ptr->empty();
		}
		nonseq_info->reset();
		return ret2;//avoid continue to exec single sentence.
	}
	if(!nonseq_info->non_seq_struct_depth && ret2 != OK_NONSEQ_INPUTING && ret1 != OK_FUNC_DEFINE || ret1 == OK_FUNC_INPUTING && ret2 != OK_NONSEQ_INPUTING && !nonseq_info->non_seq_struct_depth) {
	//if(str[0] != '}') {
		ret1 = sentence_exec(node_ptr, count, true);
		return ret1;
	}
	return ERROR_NO;
}

int c_interpreter::nonseq_mid_gen_mid_code(node_attribute_t* node_ptr, int count)
{
	mid_code* mid_code_ptr;
	int cur_depth = nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth;
	mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
	mid_code_ptr->ret_operator = CTL_BRANCH;
	this->cur_mid_code_stack_ptr->push();
	nonseq_info->row_info_node[nonseq_info->row_num].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
	for(int i=nonseq_info->row_num-1; i>=0; i--) {
		if(nonseq_info->row_info_node[i].non_seq_depth == cur_depth	&& nonseq_info->row_info_node[i].non_seq_info == 0) {
			mid_code_ptr = nonseq_info->row_info_node[i].post_info_b + (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
			mid_code_ptr->opda_addr++;
			nonseq_info->row_info_node[i].finish_flag = 1;
			break;
		}
	}
	return ERROR_NO;
}

int c_interpreter::nonseq_end_gen_mid_code(int row_num, node_attribute_t* node_ptr, int count)
{
	int i;
	int cur_depth = nonseq_info->row_info_node[row_num].non_seq_depth;
	if(count == 0)return ERROR_NO;
	mid_code *mid_code_ptr;
	if(node_ptr[0].node_type != TOKEN_KEYWORD_NONSEQ && !(node_ptr[0].node_type == TOKEN_OTHER && node_ptr[0].data == L_BIG_BRACKET)) {
		if(!nonseq_info->row_info_node[nonseq_info->row_num].finish_flag) {
			nonseq_info->row_info_node[nonseq_info->row_num].finish_flag = 1;
		}
	}
	for(i=row_num; i>=0; i--) {
		if(nonseq_info->row_info_node[i].non_seq_depth >= cur_depth 
			&& (nonseq_info->row_info_node[i].non_seq_info == 0 || nonseq_info->row_info_node[i].non_seq_info == 2) 
			&& !nonseq_info->row_info_node[i].finish_flag) {
			switch(nonseq_info->row_info_node[i].nonseq_type) {
			case NONSEQ_KEY_IF:
				mid_code_ptr = nonseq_info->row_info_node[i].post_info_b + (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
				mid_code_ptr->opda_addr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr() - mid_code_ptr;
				nonseq_info->row_info_node[i].finish_flag = 1;
				break;
			case NONSEQ_KEY_ELSE:
				mid_code_ptr = nonseq_info->row_info_node[i].post_info_b + (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
				mid_code_ptr->opda_addr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr() - mid_code_ptr;
				nonseq_info->row_info_node[i].finish_flag = 1;
				break;
			case NONSEQ_KEY_FOR:
			{
				int token_count;
				node_attribute_t node = {0, TOKEN_OPERATOR, opt_prio[OPT_R_SMALL_BRACKET], OPT_R_SMALL_BRACKET, 0, 0};
				token_count = this->generate_token_list(nonseq_info->row_info_node[i].row_ptr, nonseq_info->row_info_node[i].row_len);
				int count = find_token_with_bracket_level(this->token_node_ptr + nonseq_info->row_info_node[i].post_info_c + 1, token_count, &node, 0);
				mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
				for(mid_code *j=(mid_code*)cur_mid_code_stack_ptr->get_base_addr()+nonseq_info->row_info_node[i].post_info_b+nonseq_info->row_info_node[i].post_info_a; j<=mid_code_ptr; j++) {
					if(j->ret_operator == CTL_CONTINUE && j->opda_addr == 0)
						j->opda_addr = mid_code_ptr - j;
				}
				this->generate_mid_code(this->token_node_ptr + nonseq_info->row_info_node[i].post_info_c + 1, count, false);
				mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
				mid_code_ptr->ret_operator = CTL_BRANCH;
				mid_code_ptr->opda_addr = ((mid_code*)cur_mid_code_stack_ptr->get_base_addr() + nonseq_info->row_info_node[i].post_info_b) - mid_code_ptr;
				this->cur_mid_code_stack_ptr->push();
				mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
				for(mid_code *j=(mid_code*)cur_mid_code_stack_ptr->get_base_addr()+nonseq_info->row_info_node[i].post_info_b+nonseq_info->row_info_node[i].post_info_a; j<=mid_code_ptr; j++) {
					if(j->ret_operator == CTL_BREAK && j->opda_addr == 0)
						j->opda_addr = mid_code_ptr - j;
				}
				mid_code_ptr = (mid_code*)cur_mid_code_stack_ptr->get_base_addr() + nonseq_info->row_info_node[i].post_info_b + nonseq_info->row_info_node[i].post_info_a;
				mid_code_ptr->opda_addr = (mid_code*)cur_mid_code_stack_ptr->get_current_ptr() - mid_code_ptr;
				nonseq_info->row_info_node[i].finish_flag = 1;
				break;
			}
			case NONSEQ_KEY_WHILE:
				mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
				mid_code_ptr->ret_operator = CTL_BRANCH;
				mid_code_ptr->opda_addr = ((mid_code*)cur_mid_code_stack_ptr->get_base_addr() + nonseq_info->row_info_node[i].post_info_b) - mid_code_ptr;
				this->cur_mid_code_stack_ptr->push();
				mid_code_ptr = nonseq_info->row_info_node[i].post_info_b + nonseq_info->row_info_node[i].post_info_a + (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
				mid_code_ptr->opda_addr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr() - mid_code_ptr;
				nonseq_info->row_info_node[i].finish_flag = 1;
				break;
			case NONSEQ_KEY_DO:
				this->generate_mid_code(node_ptr + 1, count - 1, true);
				mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
				mid_code_ptr->ret_operator = CTL_BRANCH_TRUE;
				mid_code_ptr->opda_addr = ((mid_code*)cur_mid_code_stack_ptr->get_base_addr() + nonseq_info->row_info_node[i].post_info_b) - mid_code_ptr;
				this->cur_mid_code_stack_ptr->push();
				break;
			default:
				break;
			}
		}
	}
	return ERROR_NO;
}

int c_interpreter::nonseq_start_gen_mid_code(node_attribute_t *node_ptr, int count, int non_seq_type)
{
	mid_code *mid_code_ptr;
	int ret = ERROR_NO;
	uint mid_code_count;
	uint mid_code_count_of_for;
	if((node_ptr[1].node_type != TOKEN_OPERATOR || node_ptr[1].data != OPT_L_SMALL_BRACKET) && node_ptr[0].value.int_value != NONSEQ_KEY_DO && node_ptr[0].value.int_value != NONSEQ_KEY_ELSE) {
		error("Lack of bracket\n");
		return ERROR_NONSEQ_GRAMMER;
	}
	mid_code_count = this->cur_mid_code_stack_ptr->get_count();
	switch(non_seq_type) {
	case NONSEQ_KEY_IF:
		ret = this->generate_mid_code(node_ptr + 1, count - 1, false);
		if(ret)
			break;
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
		mid_code_ptr->ret_operator = CTL_BRANCH_FALSE;
		this->cur_mid_code_stack_ptr->push();
		break;
	case NONSEQ_KEY_FOR:
	{
		int first_flag_pos, second_flag_pos, third_flag_pos;
		node_attribute_t node = {0, TOKEN_OPERATOR, opt_prio[OPT_EDGE], OPT_EDGE, 0, 0};
		first_flag_pos = find_token_with_bracket_level(node_ptr + 1, count - 1, &node, 1);
		second_flag_pos = find_token_with_bracket_level(node_ptr + first_flag_pos + 2, count - 2 - first_flag_pos, &node, 0);
		nonseq_info->row_info_node[nonseq_info->row_num].post_info_c = first_flag_pos + second_flag_pos + 2;
		if(first_flag_pos == -1 || second_flag_pos == -1)
			return ERROR_NONSEQ_GRAMMER;
		ret = this->generate_mid_code(node_ptr + 2, first_flag_pos - 1, false);
		if(ret)
			break;
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();//循环条件判断处中间代码地址
		ret = this->generate_mid_code(node_ptr + first_flag_pos + 2, second_flag_pos, false);
		if(ret)
			break;
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num].post_info_a = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr() - nonseq_info->row_info_node[nonseq_info->row_num].post_info_b;//循环体处距循环条件中间代码处相对偏移
		mid_code_ptr->ret_operator = CTL_BRANCH_FALSE;
		this->cur_mid_code_stack_ptr->push();
		mid_code_count_of_for = this->cur_mid_code_stack_ptr->get_count();
		node.data = OPT_R_SMALL_BRACKET;
		node.value_type = opt_prio[node.data];
		third_flag_pos = find_token_with_bracket_level(node_ptr + nonseq_info->row_info_node[nonseq_info->row_num].post_info_c + 1, count - (nonseq_info->row_info_node[nonseq_info->row_num].post_info_c + 1), &node, 0);
		ret = this->generate_mid_code(node_ptr + nonseq_info->row_info_node[nonseq_info->row_num].post_info_c + 1, third_flag_pos, false);
		while(this->cur_mid_code_stack_ptr->get_count() > mid_code_count_of_for) {
			mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->pop();
			kmemset(mid_code_ptr, 0, sizeof(mid_code));
		}
		break;
	}
	case NONSEQ_KEY_WHILE:
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
		ret = this->generate_mid_code(node_ptr + 1, count - 1, false);
		if(ret)
			break;
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num].post_info_a = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr() - nonseq_info->row_info_node[nonseq_info->row_num].post_info_b;//循环体处距循环条件中间代码处相对偏移
		mid_code_ptr->ret_operator = CTL_BRANCH_FALSE;
		this->cur_mid_code_stack_ptr->push();
		break;
	case NONSEQ_KEY_DO:
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
		break;
	}
	if(ret) {
		while(this->cur_mid_code_stack_ptr->get_count() > mid_code_count) {
			mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->pop();
			kmemset(mid_code_ptr, 0, sizeof(mid_code));
		}
	}
	return ret;
}

int c_interpreter::struct_analysis(node_attribute_t* node_ptr, uint count)
{
	int is_varity_declare;
	if(node_ptr[0].node_type == TOKEN_KEYWORD_TYPE)
		is_varity_declare = node_ptr[0].value.int_value;
	else
		is_varity_declare = 0;
	if(this->struct_info_set.declare_flag) {
		if(this->struct_info_set.struct_begin_flag) {
			struct_info_set.struct_begin_flag = 0;
			if(!(node_ptr[0].node_type == TOKEN_OTHER && node_ptr[0].data == L_BIG_BRACKET)) {
				struct_info_set.declare_flag = 0;
				error("struct definition error\n");
				return ERROR_STRUCT_ERROR;
			} else {
				return OK_STRUCT_INPUTING;
			}
		} else {
			stack *varity_stack_ptr = this->struct_declare->current_node->varity_stack_ptr;
			if(node_ptr[0].node_type == TOKEN_OTHER && node_ptr[0].data == R_BIG_BRACKET) {
				struct_info_set.declare_flag = 0;
				this->struct_declare->current_node->struct_size = make_align(this->struct_info_set.current_offset, PLATFORM_WORD_LEN);
				vrealloc(varity_stack_ptr->get_base_addr(), varity_stack_ptr->get_count() * sizeof(varity_info));
				return OK_STRUCT_FINISH;
			} else {
				char varity_name[32];
				int type, remain_count = count, node_count = count, complex_node_count, varity_size, align_size;
				struct_info *struct_info_ptr;
				PLATFORM_WORD *varity_complex_ptr;
				varity_info* new_node_ptr = (varity_info*)varity_stack_ptr->get_current_ptr();
				type = basic_type_check(node_ptr, node_count, struct_info_ptr);
				if(type < 0) {     
					error("arg type error.\n");
					return ERROR_FUNC_ARG_LIST;
				}
				node_count = remain_count -= node_count;
				while(remain_count > 0) {
					node_count = remain_count;
					complex_node_count = get_varity_type(node_ptr, node_count, varity_name, type, struct_info_ptr, varity_complex_ptr);
					if(type == STRUCT)
						align_size = PLATFORM_WORD_LEN;
					else
						align_size = get_element_size(complex_node_count, varity_complex_ptr);
					varity_size = get_varity_size(0, varity_complex_ptr, complex_node_count);
					this->struct_info_set.current_offset = make_align(this->struct_info_set.current_offset, align_size);
					new_node_ptr->arg_init(varity_name, varity_size, complex_node_count, varity_complex_ptr, (void*)this->struct_info_set.current_offset);
					this->struct_info_set.current_offset += varity_size;
					varity_stack_ptr->push();
					node_ptr += node_count;
					remain_count -= node_count;
				}
				//this->struct_declare->save_sentence(str, len);
				return OK_STRUCT_INPUTING;
			}
		}
	}
	if(is_varity_declare == STRUCT) {
		if(count != 2)
			return OK_STRUCT_NOSTRUCT;
		if(node_ptr[1].node_type != TOKEN_NAME) {
			error("Wrong struct define.\n");
			return ERROR_STRUCT_NAME;
		}
		
		int symbol_begin_pos, ptr_level = 0;
		int keylen = kstrlen(type_key[is_varity_declare]);
		stack* arg_stack;
		this->struct_info_set.declare_flag = 1;
		this->struct_info_set.struct_begin_flag = 1;
		this->struct_info_set.current_offset = 0;
		int i = keylen + 1;
		symbol_begin_pos = i;
		varity_info* arg_node_ptr = (varity_info*)dmalloc(sizeof(varity_info) * MAX_VARITY_COUNT_IN_STRUCT, "struct arg info");
		arg_stack = (stack*)dmalloc(sizeof(stack), "struct arg stack");
		stack tmp_stack(sizeof(varity_info), arg_node_ptr, MAX_VARITY_COUNT_IN_STRUCT);
		kmemcpy(arg_stack, &tmp_stack, sizeof(stack));
		this->struct_declare->declare(node_ptr[1].value.ptr_value, arg_stack);
		return OK_STRUCT_INPUTING;

	}
	return OK_STRUCT_NOSTRUCT;
}

int c_interpreter::basic_type_check(node_attribute_t *node_ptr, int &count, struct_info *&struct_info_ptr)
{
	int is_varity_declare, total_token_len = 0;
	char struct_name[MAX_VARITY_NAME_LEN];
	if(node_ptr[0].node_type == TOKEN_KEYWORD_TYPE) {
		is_varity_declare = node_ptr[0].value.int_value;
	} else
		is_varity_declare = ERROR_NO_VARITY_FOUND;
	struct_info *struct_node_ptr = 0;
	if(is_varity_declare > 0) {
		if(is_varity_declare == STRUCT) {
			if(count < 2) {
				error("Wrong struct name.\n");
				return ERROR_STRUCT_NONEXIST;
			}
			if(node_ptr[1].node_type == TOKEN_NAME) {
				kstrcpy(struct_name, node_ptr[1].value.ptr_value);
				struct_node_ptr = this->struct_declare->find(struct_name);
				if(!struct_node_ptr) {
					error("There is no struct called %s.\n", struct_name);
					return ERROR_STRUCT_NONEXIST;
				}
			} else {
				error("Wrong struct name.\n");
				return ERROR_STRUCT_NONEXIST;
			}
			struct_info_ptr = struct_node_ptr;
			count = 2;
		}
		count = 1;
		return is_varity_declare;
	} else {
		return ERROR_NO_VARITY_FOUND;
	}
}

int c_interpreter::get_varity_type(node_attribute_t *node_ptr, int &count, char *name, int basic_type, struct_info *info, PLATFORM_WORD *&ret_info)
{
	int is_varity_declare = basic_type, total_len = 0;
	sentence_analysis_data_struct_t *analysis_data_struct_ptr = &this->interprete_need_ptr->sentence_analysis_data_struct;
	node_attribute_t *stack_top_node_ptr;
	list_stack expression_tmp_stack;
	list_stack expression_final_stack;
	//解析复杂变量类型///////////////
	int v_count = count;//+1避免嵌套调用时破坏正在进行类型转换还没push的。
	int token_flag = 0, array_flag = 0, branch_flag = 0;
	int varity_basic_type = 0;
	int arg_count = 0, non_complex_count = 0;
	PLATFORM_WORD arg_stack_addr[3];
	list_stack *cur_stack_ptr = &expression_tmp_stack;
	this->interprete_need_ptr->sentence_analysis_data_struct.last_token.node_type = TOKEN_OPERATOR;
	this->interprete_need_ptr->sentence_analysis_data_struct.last_token.data = OPT_EDGE;
	name[0] = '\0';
	for(int i=0; i<=v_count; i++) {
		if(i == v_count) {
			branch_flag = 1;
			goto varity_end;
		}
		analysis_data_struct_ptr->node_struct[i].value = &node_ptr[i];
		if(node_ptr[i].node_type == TOKEN_OPERATOR) {
			stack_top_node_ptr = (node_attribute_t*)expression_tmp_stack.get_lastest_element()->value;
			if(node_ptr[i].data == OPT_L_SMALL_BRACKET) {
				expression_tmp_stack.push(&analysis_data_struct_ptr->node_struct[i]);
			} else if(node_ptr[i].data == OPT_R_SMALL_BRACKET) {
				while(1) {//TODO:确认这里是否需要改token_flag；没有函数指针时不用是可以的，不知道加入以后会不会有问题
					stack_top_node_ptr = (node_attribute_t*)expression_tmp_stack.get_lastest_element()->value;
					if(stack_top_node_ptr->node_type == TOKEN_OPERATOR && stack_top_node_ptr->data == OPT_L_SMALL_BRACKET)
						break;
					expression_final_stack.push(expression_tmp_stack.pop());
				}
				expression_tmp_stack.pop();
				if(array_flag)
					goto array_def;
			} else if(node_ptr[i].data == OPT_INDEX) {
				array_flag = 1;
				cur_stack_ptr->push(&analysis_data_struct_ptr->node_struct[i]);
			} else if(node_ptr[i].data == OPT_R_MID_BRACKET) {
				array_def:
				array_flag = 0;
				((node_attribute_t*)expression_final_stack.get_lastest_element()->value)->value.int_value = this->interprete_need_ptr->sentence_analysis_data_struct.last_token.value.int_value;
			} else if(node_ptr[i].data == OPT_MUL || node_ptr[i].data == OPT_PTR_CONTENT) {
				cur_stack_ptr->push(&analysis_data_struct_ptr->node_struct[i]);
				node_ptr[i].data = OPT_PTR_CONTENT;
			} else if(node_ptr[i].data == OPT_COMMA || node_ptr[i].data == OPT_EDGE || node_ptr[i].data == OPT_ASSIGN) {
varity_end:
				while(expression_tmp_stack.get_count()) {//TODO:确认这里是否需要改token_flag；没有函数指针时不用是可以的，不知道加入以后会不会有问题
					stack_top_node_ptr = (node_attribute_t*)expression_tmp_stack.get_lastest_element()->value;
					expression_final_stack.push(expression_tmp_stack.pop());
				}
				int node_count = expression_final_stack.get_count();
				PLATFORM_WORD *complex_info = 0, *cur_complex_info_ptr;
				int basic_info_node_count = 1;
				if(is_varity_declare == STRUCT)
					basic_info_node_count++;
				if(node_count) {
					complex_info = (PLATFORM_WORD*)dmalloc((node_count + arg_count + 1 + basic_info_node_count) * sizeof(PLATFORM_WORD), "varity type info"); //+基本类型
					cur_complex_info_ptr = complex_info + node_count + arg_count + basic_info_node_count;
					node *head = expression_final_stack.get_head();
					arg_count = 0;
					for(int n=0; n<node_count; n++) {
						head = head->right;
						node_attribute_t *complex_attribute = (node_attribute_t*)head->value;
						if(complex_attribute->node_type == TOKEN_ARG_LIST) {
							*cur_complex_info_ptr-- = (COMPLEX_ARG << COMPLEX_TYPE_BIT);
							*cur_complex_info_ptr = arg_stack_addr[arg_count++];
						} else if(complex_attribute->data == OPT_INDEX) {
							*cur_complex_info_ptr = (COMPLEX_ARRAY << COMPLEX_TYPE_BIT) | complex_attribute->value.int_value;
						} else if(complex_attribute->data == OPT_PTR_CONTENT) {
							complex_attribute->data = OPT_MUL;
							*cur_complex_info_ptr = (COMPLEX_PTR << COMPLEX_TYPE_BIT);
						}
						cur_complex_info_ptr--;
					}
					*cur_complex_info_ptr-- = (COMPLEX_BASIC << COMPLEX_TYPE_BIT) | is_varity_declare;
					if(is_varity_declare == STRUCT) {
						*cur_complex_info_ptr-- = info->type_info_ptr[1];//TODO:或许有更好办法
					}
					int type_index = varity_type_stack.find(node_count + basic_info_node_count, complex_info);
					if(type_index >= 0) {
						ret_info = (PLATFORM_WORD*)varity_type_stack.type_info_addr[type_index];
						vfree(complex_info);
					} else {
						ret_info = complex_info;
						varity_type_stack.push(node_count + basic_info_node_count, complex_info);
					}
				} else {
					if(is_varity_declare == STRUCT)
						ret_info = info->type_info_ptr;
					else
						ret_info = basic_type_info[is_varity_declare];
				}
				////////
				this->interprete_need_ptr->sentence_analysis_data_struct.last_token.node_type = TOKEN_OPERATOR;
				this->interprete_need_ptr->sentence_analysis_data_struct.last_token.data = node_ptr[i].data;
				count = i + (branch_flag ? 0 : 1);
				return basic_info_node_count + node_count + arg_count;
			} else if(node_ptr[i].data == OPT_CALL_FUNC) {
			} else {
#if !DYNAMIC_ARRAY_EN
				error("Not allowed to use dynamic array.\n");
#endif
			}
		} else if(node_ptr[i].node_type == TOKEN_NAME) {
			token_flag = 1;
			cur_stack_ptr = &expression_final_stack;
			kstrcpy(name, node_ptr[i].value.ptr_value);
			non_complex_count++;
		} else if(node_ptr[i].node_type == TOKEN_CONST_VALUE) {
			non_complex_count++;
			if(!array_flag) {

			}
		} else if(node_ptr[i].node_type == TOKEN_ARG_LIST) {
			cur_stack_ptr->push(&analysis_data_struct_ptr->node_struct[i]);
			arg_stack_addr[arg_count++] = (PLATFORM_WORD)node_ptr[i].value.ptr_value;
		}
		this->interprete_need_ptr->sentence_analysis_data_struct.last_token = node_ptr[i];
	}
	return ERROR_NO_VARITY_FOUND;
}

int c_interpreter::varity_declare_analysis(node_attribute_t* node_ptr, int count)
{
	int is_varity_declare, basic_type_count = count, ret;
	char varity_name[32];
	int varity_special_flag = 0;//1: external 2: global
	if(node_ptr->node_type == TOKEN_SPECIFIER) {
		if(node_ptr->data == SPECIFIER_EXTERN) {
			varity_special_flag = 1;
		} else if(node_ptr->data == SPECIFIER_DELETE) {
			int name_flag = 1;
			node_ptr++;
			while(--count) {
				if(name_flag && node_ptr->node_type == TOKEN_NAME) {
					this->varity_declare->undeclare(node_ptr->value.ptr_value);
					name_flag = 0;
				} else if(!name_flag && node_ptr->node_type == TOKEN_OPERATOR) {
					name_flag = 1;
				} else {
					error("error use delete\n");
				}
				node_ptr++;
			}
			return OK_VARITY_DECLARE;
		}
		node_ptr++;
		count--;
	}
	struct_info* struct_node_ptr = 0;
	is_varity_declare = basic_type_check(node_ptr, basic_type_count, struct_node_ptr);
	count -= basic_type_count;
	node_ptr += basic_type_count;
	if(is_varity_declare >= 0) {
		int complex_node_count, varity_size, align_size;
		PLATFORM_WORD *varity_complex_ptr;
		varity_info *new_varity_ptr;
		while(count > 0) {
			int complex_part_count = count;
			complex_node_count = get_varity_type(node_ptr, complex_part_count, varity_name, is_varity_declare, struct_node_ptr, varity_complex_ptr);
			if(is_varity_declare == STRUCT)
				align_size = PLATFORM_WORD_LEN;//TODO:struct内最长对齐长度
			else
				align_size = get_element_size(complex_node_count, varity_complex_ptr);
			varity_size = get_varity_size(0, varity_complex_ptr, complex_node_count);
			if(!varity_name[0]) {
				error("Wrong varity name.\n");
				return ERROR_VARITY_NAME;
			}
			if(varity_special_flag == 1) {
				ret = this->varity_declare->declare(VARITY_SCOPE_EXTERNAL, varity_name, varity_size, complex_node_count, varity_complex_ptr);
			} else if(varity_special_flag == 2) {
				ret = this->varity_declare->declare(VARITY_SCOPE_GLOBAL, varity_name, varity_size, complex_node_count, varity_complex_ptr);
			} else {
				if(this->varity_global_flag == VARITY_SCOPE_GLOBAL) {
					ret = this->varity_declare->declare(VARITY_SCOPE_GLOBAL, varity_name, varity_size, complex_node_count, varity_complex_ptr);
					new_varity_ptr = (varity_info*)this->varity_declare->global_varity_stack->get_lastest_element();
				} else {
					ret = this->varity_declare->declare(VARITY_SCOPE_LOCAL, varity_name, varity_size, complex_node_count, varity_complex_ptr);
					new_varity_ptr = (varity_info*)this->varity_declare->local_varity_stack->get_lastest_element();
				}
			}
			if(!varity_complex_ptr[0]) {
				varity_complex_ptr[0] = 1;//起始引用次数1
			}
			node_ptr += complex_part_count;
			count -= complex_part_count;
			if(this->interprete_need_ptr->sentence_analysis_data_struct.last_token.data == OPT_ASSIGN) {//TODO: generate mid code from varity_name to ;
				if(varity_special_flag == 1) {
					error("external variable cannot be assigned.\n");
					return ERROR_ASSIGN;
				}
				node_attribute_t node = {0, TOKEN_OPERATOR, opt_prio[OPT_COMMA], OPT_COMMA, 0, 0};
				int exp_len = find_token_with_bracket_level(node_ptr, count, &node, 0);
				if(exp_len == -1) {
					node.data = OPT_EDGE;
					node.value_type = opt_prio[OPT_EDGE];
					exp_len = find_token_with_bracket_level(node_ptr, count + 1, &node, 0);
				}
				if(new_varity_ptr->get_type() != ARRAY) {
					generate_mid_code(node_ptr, exp_len, false);
					mid_code *code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
					kmemcpy(&code_ptr->opdb_addr, &(code_ptr - 1)->opda_addr, sizeof(code_ptr->opda_addr) + sizeof(code_ptr->opda_operand_type) + sizeof(code_ptr->double_space1) + sizeof(code_ptr->opda_varity_type));
					if(this->varity_global_flag == VARITY_SCOPE_LOCAL) {
						code_ptr->ret_addr = code_ptr->opda_addr = (int)new_varity_ptr->get_content_ptr();
						code_ptr->ret_operand_type = code_ptr->opda_operand_type = OPERAND_L_VARITY;
					} else {
						code_ptr->ret_addr = code_ptr->opda_addr = (int)new_varity_ptr->get_name();
						code_ptr->ret_operand_type = code_ptr->opda_operand_type = OPERAND_G_VARITY;
					}
					code_ptr->ret_varity_type = code_ptr->opda_varity_type = new_varity_ptr->get_type();
					code_ptr->data = new_varity_ptr->get_element_size();
					code_ptr->ret_operator = OPT_ASSIGN;
					this->cur_mid_code_stack_ptr->push();
				} else {//数组赋值
				}
				node_ptr += exp_len + 1;
				count -= exp_len + 1;
			}
		}
		return OK_VARITY_DECLARE;
	}
	return ERROR_NO;
}

int c_interpreter::sentence_exec(node_attribute_t* node_ptr, uint count, bool need_semicolon)
{
	int ret;
	if(!count)
		return ERROR_NO;
	if(node_ptr[0].node_type == TOKEN_OTHER)
		return ERROR_NO;
	if(need_semicolon) {
		count--;
		if(node_ptr[count].data != OPT_EDGE || node_ptr[count].node_type != TOKEN_OPERATOR) {
			error("Missing ;\n");
			return ERROR_SEMICOLON;
		}
	}
	int key_word_ret = varity_declare_analysis(node_ptr, count);
	if(!key_word_ret) {
		ret = this->generate_mid_code(node_ptr, count, false);
		if(ret) return ret;
	}
	if(this->exec_flag) {
		int mid_code_count = this->cur_mid_code_stack_ptr->get_count();
		ret = this->ulink(this->cur_mid_code_stack_ptr, LINK_ADDR);
		if(ret)
			return ret;
		this->print_code((mid_code*)this->cur_mid_code_stack_ptr->get_base_addr(), this->cur_mid_code_stack_ptr->get_count(), INTERPRETER_DEBUG);
		this->exec_mid_code((mid_code*)this->cur_mid_code_stack_ptr->get_base_addr(), mid_code_count);
		this->cur_mid_code_stack_ptr->empty();
	}
	return ERROR_NO;
}

int c_interpreter::get_token(char *str, node_attribute_t *info)
{
	int i = 0, real_token_pos, float_flag = 0;
	char *symbol_ptr = token_fifo.get_wptr() + (char*)token_fifo.get_base_addr();
	while(str[i] == ' ' || str[i] == '\t')i++;
	real_token_pos = i;
	kmemset(info, 0, sizeof(node_attribute_t));
	info->pos = i;
	if(is_letter(str[i])) {
		i++;
		for(int j=sizeof(type_key)/sizeof(type_key[0])-1; j>=0; j--) {//避免多段字符串构成的类型检测不到，使用strncmp
			if(!kstrncmp(str + real_token_pos, type_key[j], type_len[j])) {
				if(is_valid_c_char(str[real_token_pos + type_len[j]]))
					continue;//unsigned long long1要识别为ulong型long1，所以不能break
				info->value.int_value = j;
				info->node_type = TOKEN_KEYWORD_TYPE;
				return type_len[j] + real_token_pos;
			}
		}
		while(is_valid_c_char(str[i]))i++;
		token_fifo.write(str + real_token_pos, i - real_token_pos);
		token_fifo.write("\0", 1);
		for(int j=0; j<sizeof(non_seq_key)/sizeof(non_seq_key[0]); j++) {
			if(!kstrcmp(symbol_ptr, non_seq_key[j])) {
				info->value.int_value = j;
				info->node_type = TOKEN_KEYWORD_NONSEQ;
				return i;
			}
		}
		for(int j=0; j<sizeof(ctl_key)/sizeof(ctl_key[0]); j++) {
			if(!kstrcmp(symbol_ptr, ctl_key[j])) {
				info->value.int_value = j + 1;
				info->node_type = TOKEN_KEYWORD_CTL;
				return i;
			}
		}
		if(!kstrcmp(symbol_ptr, "sizeof")) {
			info->node_type = TOKEN_OPERATOR;
			info->data = OPT_SIZEOF;
			info->value_type = 2;
			return i;
		}
		if(!kstrcmp(symbol_ptr, "exist")) {
			info->node_type = TOKEN_OPERATOR;
			info->data = OPT_EXIST;
			info->value_type = 2;
			return i;
		}
		if(!kstrcmp(symbol_ptr, "extern")) {
			info->node_type = TOKEN_SPECIFIER;
			info->data = SPECIFIER_EXTERN;
			return i;
		}
		if(!kstrcmp(symbol_ptr, "delete")) {
			info->node_type = TOKEN_SPECIFIER;
			info->data = SPECIFIER_DELETE;
			return i;
		}
		string_info *string_ptr;
		string_ptr = (string_info*)name_stack.find(symbol_ptr);
		if(!string_ptr) {
			string_info tmp;
			info->value.ptr_value = name_fifo.write(symbol_ptr);
			tmp.set_name(info->value.ptr_value);
			name_stack.push(&tmp);
		} else {
			info->value.ptr_value = string_ptr->get_name();
		}
		//info->value.ptr_value = symbol_ptr;
		info->node_type = TOKEN_NAME;
		return i;
	} else if(kisdigit(str[i]) || (str[i] == '.' && kisdigit(str[i + 1]))) {
		if(str[i] == '0' && (str[i + 1] == 'x' || str[i + 1] == 'X')) {
			if(!(str[i + 2] >= '0' && str[i + 2] <= '9' || str[i + 2] >= 'a' && str[i + 2] <= 'f' || str[i + 2] >= 'A' && str[i + 2] <= 'F')) {
				error("Illegal hex number.\n");
				return ERROR_TOKEN;
			}
			i += 2;
			while(1) {
				if(!(str[i] >= '0' && str[i] <= '9' || str[i] >= 'a' && str[i] <= 'f' || str[i] >= 'A' && str[i] <= 'F')) {
					goto int_value_handle;
				}
				i++;
			}
		}
		i++;
		while(kisdigit(str[i]))i++;
		if(str[i] == '.') {
			i++;
			float_flag = 1;
			while(kisdigit(str[i]))i++;
		}
		if(str[i] == 'e' || str[i] == 'E') {
			i++;
			float_flag = 1;
			if(str[i] == '-' || str[i] == '+')
				i++;
			if(!kisdigit(str[i])) {
				error("Illegal float.\n");
				info->node_type = TOKEN_ERROR;
				return ERROR_TOKEN;
			}
			while(kisdigit(str[i]))i++;
		}
		if(is_letter(str[i])) {
			error("Illegal const value or name.\n");
			info->node_type = TOKEN_ERROR;
			return ERROR_TOKEN;
		}
		if(float_flag) {
			info->value_type = DOUBLE;
			info->value.double_value = y_atof(str, i);
			info->node_type = TOKEN_CONST_VALUE;
			return i;
		}
int_value_handle:
		info->value.int_value = y_atoi(str, i);
		if(str[i] == 'u') {
			info->value_type = U_INT;
			i++;
		} else {
			info->value_type = INT;
		}
		info->node_type = TOKEN_CONST_VALUE;
		return i;
	} else if(str[i] == '"') {
		int count = 0;
		int pos = ++i;
		while(str[i]) {
			if(str[i] == '\\') {
				char ch;
				int escape_len = get_escape_char(str + i, ch);
				if(escape_len > 0)
					i += escape_len - 1;
				else {
					info->node_type = TOKEN_ERROR;
					return ERROR_TOKEN;
				}
			} else if(str[i] == '"') {
				char *p;
				for(int j=0; j<count; j++) {
					if(str[pos] == '\\') {
						char ch;
						int escape_len = get_escape_char(str + pos, ch);
						pos += escape_len;
						symbol_ptr[j] = ch;
					} else {
						symbol_ptr[j] = str[pos++];
					}
				}
				symbol_ptr[count] = 0;
				string_info *str_node_ptr = (string_info*)string_stack.find(symbol_ptr);
				if(str_node_ptr) {
					p = (char*)str_node_ptr->get_name();
					info->value.int_value = str_node_ptr - (string_info*)string_stack.get_base_addr();
				} else {
					string_info str_info;
					p = (char*)dmalloc(count + 1, "string space");
					str_info.set_name(p);
					//str_info.index = string_stack.get_count();
					info->value.int_value = string_stack.get_count();
					kstrcpy(p, symbol_ptr);
					string_stack.push(&str_info);
				}
				info->node_type = TOKEN_STRING;
				return i + 1;
			}
			i++;
			count++;
		}
	} else if(str[i] == '\'') {
		char character;
		int ret = 1;
		i++;
		if(str[i] == '\\') {
			ret = get_escape_char(str + i, character);
		} else {
			character = str[i];
		}
		i += ret;
		if(str[i] == '\'') {
			info->value_type = CHAR;
			info->value.int_value = character;
			info->node_type = TOKEN_CONST_VALUE;
			return i + 1;
		} else {
			info->node_type = TOKEN_ERROR;
			error("\' operator error.\n");
			return ERROR_TOKEN;
		}
	} else if(str[i] == '{' || str[i] == '}') {
		info->node_type = TOKEN_OTHER;
		if(str[i] == '{')
			info->data = L_BIG_BRACKET;
		else
			info->data = R_BIG_BRACKET;
		return i + 1;
	} else {
		for(int j=0; j<sizeof(opt_str)/sizeof(opt_str[0]); j++) {
			if(!kstrncmp(str + i, opt_str[j], opt_str_len[j])) {
				info->data = j;
				info->node_type = TOKEN_OPERATOR;
				info->value_type = opt_prio[j];
				return i + opt_str_len[j];
			}
		}
		for(;str[i];i++) {
			if(str[i] != ' ' || str[i] != '\t') {
				error("Token error.\n");
				info->node_type = TOKEN_ERROR;
				return ERROR_TOKEN;
			}
		}
		info->node_type = TOKEN_NONEXIST;
		return i;
	}
	return 0;
}

int c_interpreter::token_convert(node_attribute_t *node_ptr, int &count)
{
	int wptr = 0;
	for(int i=0; i<count; i++) {
		if(node_ptr[i].node_type != TOKEN_OPERATOR) {
			this->token_node_ptr[wptr++] = node_ptr[i];
			continue;
		}
		switch(node_ptr[i].data) {
			case OPT_L_SMALL_BRACKET:
			{
				int token_in_bracket;
				int type_flag = 0;
				node_attribute_t rbracket_node = {0, TOKEN_OPERATOR, opt_prio[OPT_R_SMALL_BRACKET], OPT_R_SMALL_BRACKET, 0, 0};
				char varity_name[32];
				stack *arg_stack = 0;
				PLATFORM_WORD *varity_complex_ptr;
				int complex_node_count;
				int void_flag = 0;
				token_in_bracket = find_token_with_bracket_level(node_ptr + i + 1, count - i - 1, &rbracket_node, 0);
				i++;
				for(; token_in_bracket > 0;) {
					int type;
					int v_count = token_in_bracket;		
					struct_info *struct_info_ptr;
					varity_info* arg_node_ptr;
					type = basic_type_check(node_ptr + i, v_count, struct_info_ptr);
					if(type < 0) {
						if(type_flag) {
							this->token_node_ptr[wptr].node_type = TOKEN_ERROR;
							error("Illegal arg list.\n");
							clear_arglist(arg_stack);
							return ERROR_FUNC_ARG_LIST;
						}
						i--;
						goto normal_bracket;
					} else {
						if(!type_flag) {
							type_flag = 1;
							arg_stack = (stack*)dmalloc(sizeof(stack), "TOKEN_ARG stack");
							arg_node_ptr = (varity_info*)dmalloc(sizeof(varity_info) * MAX_FUNCTION_ARGC, "TOKEN_ARG node");
							arg_stack->init(sizeof(varity_info), arg_node_ptr, MAX_FUNCTION_ARGC);
						}
					}
					i += v_count;
					v_count = token_in_bracket -= v_count;
					complex_node_count = get_varity_type(node_ptr + i, v_count, varity_name, type, struct_info_ptr, varity_complex_ptr);
					i += v_count;
					token_in_bracket -= v_count;
					if(type == VOID) {
						if(complex_node_count == 1 || (complex_node_count > 1 && GET_COMPLEX_TYPE(varity_complex_ptr[complex_node_count]) != COMPLEX_PTR)) {
							if(arg_stack->get_count() >= 1) {
								error("arg list error.\n");
								clear_arglist(arg_stack);
								this->token_node_ptr[wptr].node_type = TOKEN_ERROR;
								return ERROR_FUNC_ARG_LIST;
							}
							void_flag = true;
						}
						//void_flag = true;
					} else {
						if(void_flag) {
							error("arg cannot use void type.\n");
							clear_arglist(arg_stack);
							this->token_node_ptr[wptr].node_type = TOKEN_ERROR;
							return ERROR_FUNC_ARG_LIST;
						}
					}
					if(!void_flag) {
						arg_node_ptr->arg_init(varity_name, get_varity_size(0, varity_complex_ptr, complex_node_count), complex_node_count, varity_complex_ptr, 0);
						arg_stack->push(arg_node_ptr++);
					} else {
						if(varity_name[0] != 0) {
							error("arg cannot use void type.\n");
							clear_arglist(arg_stack);
							this->token_node_ptr[wptr].node_type = TOKEN_ERROR;
							return ERROR_FUNC_ARG_LIST;
						}
					}
				}
				if(arg_stack && arg_stack->get_count() == 1 && !varity_name[0]) {
					this->token_node_ptr[wptr].data = OPT_TYPE_CONVERT;
					this->token_node_ptr[wptr].value_type = 2;
					this->token_node_ptr[wptr].value.ptr_value = (char*)varity_complex_ptr;
					this->token_node_ptr[wptr].count = complex_node_count; //complex_arg_count
					this->token_node_ptr[wptr].node_type = TOKEN_OPERATOR;
					clear_arglist(arg_stack);
				} else {
					if(!void_flag && !arg_stack) {
						i--;
						goto normal_bracket;
					}
					this->token_node_ptr[wptr].node_type = TOKEN_OPERATOR;
					this->token_node_ptr[wptr].node_type = TOKEN_ARG_LIST;
					this->token_node_ptr[wptr].value.ptr_value = (char*)arg_stack;
					if(arg_stack)
						vrealloc(arg_stack->get_base_addr(), arg_stack->get_count() * sizeof(varity_info));
				}
				wptr++;
				continue;
normal_bracket:
				//if(i > 0 && (node_ptr[i - 1].node_type == TOKEN_NAME || node_ptr[i - 1].node_type == TOKEN_OPERATOR && (node_ptr[i - 1].data == OPT_R_SMALL_BRACKET || node_ptr[i - 1].data == OPT_R_MID_BRACKET))) {
				if(i > 0 && (node_ptr[i - 1].node_type == TOKEN_NAME || this->token_node_ptr[wptr - 1].node_type == TOKEN_OPERATOR && (this->token_node_ptr[wptr - 1].data == OPT_R_SMALL_BRACKET || this->token_node_ptr[wptr - 1].data == OPT_R_MID_BRACKET))) {
					this->token_node_ptr[wptr] = node_ptr[i];
					this->token_node_ptr[wptr++].data = OPT_CALL_FUNC;
					this->token_node_ptr[wptr++] = node_ptr[i];
					continue;
				} else {
					this->token_node_ptr[wptr++] = node_ptr[i];
				}
				break;
			}
			case OPT_PLUS:
			case OPT_MINUS:
				if(node_ptr[i - 1].node_type == TOKEN_OPERATOR 
					&& node_ptr[i - 1].data != OPT_R_SMALL_BRACKET 
					&& node_ptr[i - 1].data != OPT_L_MINUS_MINUS 
					&& node_ptr[i - 1].data != OPT_L_PLUS_PLUS 
					&& node_ptr[i - 1].data != OPT_R_PLUS_PLUS 
					&& node_ptr[i - 1].data != OPT_R_MINUS_MINUS) {
					this->token_node_ptr[wptr] = node_ptr[i];
					if(node_ptr[i].data == OPT_PLUS)
						this->token_node_ptr[wptr].data = OPT_POSITIVE;
					else
						this->token_node_ptr[wptr].data = OPT_NEGATIVE;
					this->token_node_ptr[wptr++].value_type = 2;
				} else {
					this->token_node_ptr[wptr++] = node_ptr[i];
				}
				break;
			case OPT_PLUS_PLUS:
			case OPT_MINUS_MINUS:
				if(node_ptr[i - 1].node_type == TOKEN_NAME && node_ptr[i + 1].node_type == TOKEN_NAME) {
					this->token_node_ptr[wptr++] = node_ptr[i];
					this->token_node_ptr[wptr] = node_ptr[i];
					if(node_ptr[i].data == OPT_PLUS_PLUS) {
						this->token_node_ptr[wptr - 1].data = OPT_PLUS;
						this->token_node_ptr[wptr].data = OPT_POSITIVE;
					} else {
						this->token_node_ptr[wptr - 1].data = OPT_MINUS;
						this->token_node_ptr[wptr].data = OPT_NEGATIVE;
					}
					this->token_node_ptr[wptr - 1].value_type = 4;
					this->token_node_ptr[wptr].value_type = 2;
					this->token_node_ptr[wptr++].value.long_long_value  = 0;
				} else {
					this->token_node_ptr[wptr++] = node_ptr[i];
				}
				break;
			case OPT_MUL:
			case OPT_BIT_AND:
				if(i == 0
					|| this->token_node_ptr[wptr - 1].node_type == TOKEN_OPERATOR 
					&& this->token_node_ptr[wptr - 1].data != OPT_R_SMALL_BRACKET 
					&& this->token_node_ptr[wptr - 1].data != OPT_L_MINUS_MINUS 
					&& this->token_node_ptr[wptr - 1].data != OPT_L_PLUS_PLUS
					&& this->token_node_ptr[wptr - 1].data != OPT_R_PLUS_PLUS
					&& this->token_node_ptr[wptr - 1].data != OPT_R_MINUS_MINUS){
					this->token_node_ptr[wptr] = node_ptr[i];
					if(node_ptr[i].data == OPT_MUL)
						this->token_node_ptr[wptr].data = OPT_PTR_CONTENT;
					else
						this->token_node_ptr[wptr].data = OPT_ADDRESS_OF;
					this->token_node_ptr[wptr++].value_type = 2;
				} else {
					this->token_node_ptr[wptr++] = node_ptr[i];
				}
				break;
			case OPT_L_MID_BRACKET:
				this->token_node_ptr[wptr] = node_ptr[i];
				this->token_node_ptr[wptr++].data = OPT_INDEX;
				this->token_node_ptr[wptr] = node_ptr[i];
				this->token_node_ptr[wptr++].data = OPT_L_SMALL_BRACKET;
				break;
			case OPT_R_MID_BRACKET:
				this->token_node_ptr[wptr] = node_ptr[i];
				this->token_node_ptr[wptr].data = OPT_R_SMALL_BRACKET;
				wptr++;
				break;
			default:
				this->token_node_ptr[wptr++] = node_ptr[i];
		}
	}
	return wptr;
}

void c_interpreter::print_code(mid_code *ptr, int n, int echo)
{
	if(!echo) return;
	varity_info *varity_base = (varity_info*)this->varity_declare->global_varity_stack->get_base_addr();
	for(int i=0; i<n; i++, ptr++) {
		gdbout("%03d ", i);
		if(ptr->ret_operand_type == OPERAND_T_VARITY) {
			gdbout("$%d=", ptr->ret_addr / 8);
		} else if(ptr->ret_operand_type == OPERAND_LINK_VARITY) {
			gdbout("#%d=", ptr->ret_addr / 8);
		} else if(ptr->ret_operand_type == OPERAND_G_VARITY) {
			for(uint i=0; i<this->language_elment_space.g_varity_list.get_count(); i++) {
				if(varity_base[i].get_content_ptr() == (void*)ptr->ret_addr) {
					gdbout("%s=", varity_base[i].get_name());
					break;
				}
			}
		} else if(ptr->ret_operand_type == OPERAND_L_VARITY) {
			gdbout("*(SP+%d)=", ptr->ret_addr);
		}
		if(opt_number[ptr->ret_operator] == 2 && !(ptr->ret_operator >= OPT_DEVIDE_ASSIGN && ptr->ret_operator <= OPT_BIT_OR_ASSIGN || ptr->ret_operator == OPT_ASSIGN)) {
			if(ptr->opda_operand_type == OPERAND_T_VARITY) {
				gdbout("$%d", ptr->opda_addr / 8);
			} else if(ptr->opda_operand_type == OPERAND_LINK_VARITY) {
				gdbout("#%d", ptr->opda_addr / 8);
			} else if(ptr->opda_operand_type == OPERAND_G_VARITY) {
				for(uint i=0; i<this->language_elment_space.g_varity_list.get_count(); i++) {
					if(varity_base[i].get_content_ptr() == (void*)ptr->opda_addr) {
						gdbout("%s", varity_base[i].get_name());
						break;
					}
				}
			} else if(ptr->opda_operand_type == OPERAND_L_VARITY) {
				gdbout("*(SP+%d)", ptr->opda_addr);
			} else if(ptr->opda_operand_type == OPERAND_CONST) {
				if(ptr->opda_varity_type == INT || ptr->opda_varity_type == SHORT || ptr->opda_varity_type == LONG || ptr->opda_varity_type == CHAR) {
					gdbout("%d", ptr->opda_addr);
				} else if(ptr->opda_varity_type == U_INT || ptr->opda_varity_type == U_SHORT || ptr->opda_varity_type == U_LONG || ptr->opda_varity_type == U_CHAR) {
					gdbout("%lu", ptr->opda_addr);
				} else if(ptr->opda_varity_type == FLOAT) {
					gdbout("%f", FLOAT_VALUE(&ptr->opda_addr));
				} else if(ptr->opda_varity_type == DOUBLE) {
					gdbout("%f", DOUBLE_VALUE(&ptr->opda_addr));
				}
			}
		}

		if(ptr->ret_operator < 80) {
			if(!(ptr->ret_operator >= OPT_DEVIDE_ASSIGN && ptr->ret_operator <= OPT_BIT_OR_ASSIGN || ptr->ret_operator == OPT_ASSIGN) && ptr->ret_operator <= OPT_COMMA)
				gdbout("%s", opt_str[ptr->ret_operator]);
			else {
				switch(ptr->ret_operator) {
				case OPT_CALL_FUNC:
					function_info *function_ptr = (function_info*)ptr->opda_addr;
					gdbout("CALL %s", function_ptr->get_name());
					break;
					
				}
			}
			if(ptr->opdb_operand_type == OPERAND_T_VARITY) {
				gdbout("$%d", ptr->opdb_addr / 8);
			} else if(ptr->opdb_operand_type == OPERAND_LINK_VARITY) {
				gdbout("#%d", ptr->opdb_addr / 8);
			} else if(ptr->opdb_operand_type == OPERAND_G_VARITY) {
				for(uint j=0; j<this->language_elment_space.g_varity_list.get_count(); j++) {
					if(varity_base[j].get_content_ptr() == (void*)ptr->opdb_addr) {
						gdbout("%s", varity_base[j].get_name());
						break;
					}
				}
				if(i == this->language_elment_space.g_varity_list.get_count() && ptr->opdb_varity_type == ARRAY) {
					gdbout("0x%x", ptr->opdb_addr);
				}
			} else if(ptr->opdb_operand_type == OPERAND_L_VARITY) {
				gdbout("*(SP+%d)", ptr->opdb_addr);
			} else if(ptr->opdb_operand_type == OPERAND_CONST) {
				if(ptr->opdb_varity_type == INT || ptr->opdb_varity_type == SHORT || ptr->opdb_varity_type == LONG || ptr->opdb_varity_type == CHAR) {
					gdbout("%d", ptr->opdb_addr);
				} else if(ptr->opdb_varity_type == U_INT || ptr->opdb_varity_type == U_SHORT || ptr->opdb_varity_type == U_LONG || ptr->opdb_varity_type == U_CHAR) {
					gdbout("%lu", ptr->opdb_addr);
				} else if(ptr->opdb_varity_type == FLOAT) {
					gdbout("%f", FLOAT_VALUE(&ptr->opdb_addr));
				} else if(ptr->opdb_varity_type == DOUBLE) {
					gdbout("%f", DOUBLE_VALUE(&ptr->opdb_addr));
				}
			}
		} else {
			switch(ptr->ret_operator) {
			case CTL_BRANCH:
			case CTL_RETURN:
			case CTL_BREAK:
			case CTL_CONTINUE:
			case CTL_GOTO:
				gdbout("B %d", ptr->opda_addr);
				break;
			case CTL_BRANCH_TRUE:
				gdbout("BTrue %d", ptr->opda_addr);
				break;
			case CTL_BRANCH_FALSE:
				gdbout("BFalse %d", ptr->opda_addr);
				break;
			case CTL_BXLR:
				gdbout("BXLR");
				break;
			case CTL_SP_ADD:
				gdbout("SP=SP");
				ptr->opda_addr >= 0 ? gdbout("+%d", ptr->opda_addr) : gdbout("%d", ptr->opda_addr);
			}
		}
		gdbout("\n");
		//gdbout("opt=%d,radd=%x,rtype=%d,ropd=%d,aadd=%x,atype=%d,aopd=%d,badd=%x,byte=%d,bopd=%d\n",ptr->ret_operator,ptr->ret_addr,ptr->ret_varity_type,ptr->ret_operand_type,ptr->opda_addr,ptr->opda_varity_type,ptr->opda_operand_type,ptr->opdb_addr,ptr->opdb_varity_type,ptr->opdb_operand_type);
	}
}

int c_interpreter::open_ref(char *file)
{
	int ret;
	terminal *ttybak, *ttylogbak;
	this->get_tty(&ttybak, &ttylogbak);
	this->set_tty(&fileio, 0);
	ret = fileio.init(file, "r");
	if(ret)
		return ret;
	int count = this->interprete_need_ptr->row_pretreat_fifo.get_count();
	char *pretreat_fifo_bak = (char*)dmalloc(count, "pretreat fifo bak");
	this->interprete_need_ptr->row_pretreat_fifo.read(pretreat_fifo_bak, count);

	mid_code *base_bak = (mid_code*)this->interprete_need_ptr->mid_code_stack.get_base_addr();
	int count_bak = this->interprete_need_ptr->mid_code_stack.get_count();
	mid_code *pc_bak = this->pc;
	this->interprete_need_ptr->mid_code_stack.set_base(base_bak + count_bak);
	this->interprete_need_ptr->mid_code_stack.set_count(0);

	ret = this->run_interpreter();
	this->interprete_need_ptr->mid_code_stack.set_base(base_bak);
	this->interprete_need_ptr->mid_code_stack.set_count(count_bak);
	this->pc = pc_bak;
	this->interprete_need_ptr->row_pretreat_fifo.write(pretreat_fifo_bak, count);
	vfree(pretreat_fifo_bak);
	this->set_tty(ttybak, ttylogbak);
	return ret;

}

int c_interpreter::open_eval(char *str, bool need_semicolon)
{
	int ret, len = kstrlen(str), count;
	mid_code *base_bak = (mid_code*)this->interprete_need_ptr->mid_code_stack.get_base_addr();
	int count_bak = this->interprete_need_ptr->mid_code_stack.get_count();
	mid_code *pc_bak = this->pc;
	this->interprete_need_ptr->mid_code_stack.set_base(base_bak + count_bak);
	this->interprete_need_ptr->mid_code_stack.set_count(0);
	count = this->generate_token_list(str, len);
	ret = this->sentence_exec(this->token_node_ptr, count, need_semicolon);
	this->interprete_need_ptr->mid_code_stack.set_base(base_bak);
	this->interprete_need_ptr->mid_code_stack.set_count(count_bak);
	this->pc = pc_bak;
	return ret;
}

int user_eval(char *str)
{
	return myinterpreter.open_eval(str, true);
}

int refscript(char *file)
{//TODO: protect pretreat buffer
	return myinterpreter.open_ref(file);
}

extern "C" void run_interpreter(void)
{
	kprintf("\nC program language interpreter.\n");
	myinterpreter.run_interpreter();
}

extern "C" void uc_timer_init(unsigned int index);
extern "C" void global_init(void)
{
	heapinit();
	token_fifo.init(MAX_TOKEN_BUFLEN);
	//uc_timer_init(1);
}

extern "C" void global_dispose(void)
{
	token_fifo.dispose();
}

void clear_arglist(stack *arg_stack_ptr)
{
	for(int n=0; n<arg_stack_ptr->get_count(); n++) {
		varity_info *varity_ptr = (varity_info*)arg_stack_ptr->pop();
		dec_varity_ref(varity_ptr, 1);
		vfree(varity_ptr->get_name());
	}
	vfree(arg_stack_ptr->get_base_addr());
	vfree(arg_stack_ptr);
}

