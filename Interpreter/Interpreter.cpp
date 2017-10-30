#include "interpreter.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "operator.h"
#include "string_lib.h"
#include "error.h"

tty stdio;
varity_info g_varity_node[MAX_G_VARITY_NODE];
varity_info l_varity_node[MAX_L_VARITY_NODE];
varity_info a_varity_node[MAX_A_VARITY_NODE];
stack a_varity_list(sizeof(varity_info), a_varity_node, MAX_A_VARITY_NODE);
indexed_stack l_varity_list(sizeof(varity_info), l_varity_node, MAX_L_VARITY_NODE);
stack g_varity_list(sizeof(varity_info), g_varity_node, MAX_G_VARITY_NODE);
varity c_varity(&g_varity_list, &l_varity_list, &a_varity_list);
nonseq_info_struct nonseq_info_s;
function_info function_node[MAX_FUNCTION_NODE];
stack function_list(sizeof(function_info), function_node, MAX_FUNCTION_NODE);
function c_function(&function_list);
struct_info struct_node[MAX_STRUCT_NODE];
stack struct_list(sizeof(struct_info), struct_node, MAX_STRUCT_NODE);
struct_define c_struct(&struct_list);
c_interpreter myinterpreter(&stdio, &c_varity, &nonseq_info_s, &c_function, &c_struct);

void nonseq_info_struct::reset(void)
{
	memset(this, 0, sizeof(nonseq_info_struct));
}

int c_interpreter::pre_treat(void)
{
	int spacenum = 0;
	int rptr = 0, wptr = 0, first_word = 1, space_flag = 0;
	char nowchar;
	while(nowchar = sentence_buf[rptr]) {
		if(IsSpace(nowchar)) {
			if(!first_word) {
				if(!space_flag)
					sentence_buf[wptr++] = ' ';
				space_flag = 1;
			}
		} else {
			space_flag = 0;
			first_word = 0;
			sentence_buf[wptr++] = sentence_buf[rptr];
		}
		rptr++;
	}
	sentence_buf[wptr] = 0;
	//this->tty_used->puts(sentence_buf);
	//this->tty_used->puts("\n");
	return wptr;
}

int c_interpreter::run_interpreter(void)
{
	while(1) {
		uint len;
		printf(">> ");
		len = this->row_pretreat_fifo.readline(sentence_buf);
		if(len > 0) {

		} else {
			tty_used->readline(sentence_buf);
			len = pre_treat();
		}
		this->sentence_analysis(sentence_buf, len);
	}
}

c_interpreter::c_interpreter(terminal* tty_used, varity* varity_declare, nonseq_info_struct* nonseq_info, function* function_declare, struct_define* struct_declare)
{
	this->tty_used = tty_used;
	this->varity_declare = varity_declare;
	this->nonseq_info = nonseq_info;
	this->function_declare = function_declare;
	this->struct_declare = struct_declare;
	this->function_return_value = (varity_info*)vmalloc(sizeof(varity_info));
	this->row_pretreat_fifo.set_base(this->pretreat_buf);
	this->row_pretreat_fifo.set_length(sizeof(this->pretreat_buf));
	this->non_seq_code_fifo.set_base(this->non_seq_tmp_buf);
	this->non_seq_code_fifo.set_length(sizeof(this->non_seq_tmp_buf));
	this->non_seq_code_fifo.set_element_size(1);
	this->analysis_buf_ptr = this->analysis_buf;
	this->nonseq_info->nonseq_begin_stack_ptr = 0;
	this->varity_global_flag = VARITY_SCOPE_GLOBAL;
	this->c_opt_caculate_func_list[0]=&c_interpreter::member_opt;
	this->c_opt_caculate_func_list[1]=&c_interpreter::auto_inc_opt;
	this->c_opt_caculate_func_list[2]=&c_interpreter::multiply_opt;
	this->c_opt_caculate_func_list[3]=&c_interpreter::plus_opt;
	this->c_opt_caculate_func_list[5]=&c_interpreter::relational_opt;
	this->c_opt_caculate_func_list[6]=&c_interpreter::equal_opt;
	this->c_opt_caculate_func_list[13]=&c_interpreter::assign_opt;
}

int c_interpreter::save_sentence(char* str, uint len)
{
	nonseq_info->row_info_node[nonseq_info->row_num].row_ptr = &non_seq_tmp_buf[non_seq_code_fifo.wptr];
	nonseq_info->row_info_node[nonseq_info->row_num].row_len = len;
	non_seq_code_fifo.write(str, len);
	non_seq_code_fifo.write("\n", 1);
	return 0;
}

int c_interpreter::non_seq_struct_analysis(char* str, uint len)
{
	nonseq_info->last_non_seq_check_ret = nonseq_info->non_seq_check_ret;
	if(len == 0) {
		if(nonseq_info->non_seq_struct_depth && nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] != NONSEQ_KEY_WAIT_ELSE) {
			error("blank shouldn't appear here.\n");
			nonseq_info->reset();
			return ERROR_NONSEQ_GRAMMER;
		}
	}
	nonseq_info->non_seq_check_ret = non_seq_struct_check(str);
	if(len && (nonseq_info->non_seq_check_ret || nonseq_info->non_seq_struct_depth)) {
		this->save_sentence(str, len);
		nonseq_info->row_num++;
	}
	if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_WAIT_ELSE) {
		if(nonseq_info->brace_depth == 0) {
			if(len != 0 && nonseq_info->non_seq_check_ret != NONSEQ_KEY_ELSE) {
				error("if is unmatch with else or blank\n");
				nonseq_info->reset();
				return ERROR_NONSEQ_GRAMMER;
			} else if(len == 0) {
				while(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] == 0 && this->nonseq_info->non_seq_struct_depth > 0) {
					nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth--]=0;
					if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_IF) {
						nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
						//break;
					} else if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_DO) {
						nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
						break;
					}
				}
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth = nonseq_info->non_seq_struct_depth + 1;
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 1;
				if(nonseq_info->non_seq_struct_depth == 0) {
					if(nonseq_info->non_seq_type_stack[0] != NONSEQ_KEY_WAIT_WHILE) {
						//save_sentence(str, len);
						nonseq_info->non_seq_exec = 1;
						return OK_NONSEQ_FINISH;
					} else {
						//save_sentence(str, len);
						return OK_NONSEQ_INPUTING;
					}
				}
			} else { //nonseq_info->non_seq_check_ret == NONSEQ_KEY_ELSE
				nonseq_info->row_info_node[nonseq_info->row_num - 2].non_seq_info = 3;
			}
		} else {
			if(nonseq_info->non_seq_check_ret != NONSEQ_KEY_ELSE && len != 0) {
				while(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] == 0 && this->nonseq_info->non_seq_struct_depth > 0) {
					//没清掉type_stack
					nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth--] = 0;
					if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_IF) {
						nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = 0;
						//nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
						//break;
					} else if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_DO) {
						nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
						break;
					}
				}
				nonseq_info->row_info_node[nonseq_info->row_num - 2].non_seq_depth = nonseq_info->non_seq_struct_depth + 1;
				nonseq_info->row_info_node[nonseq_info->row_num - 2].non_seq_info = 1;
			} else if(nonseq_info->non_seq_check_ret == NONSEQ_KEY_ELSE) {
				nonseq_info->row_info_node[nonseq_info->row_num - 2].non_seq_info = 3;
			}
		}
	} else if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_WAIT_WHILE) {
		if(nonseq_info->non_seq_check_ret != NONSEQ_KEY_WHILE && len != 0) {
			error("do is unmatch with while\n");
			nonseq_info->reset();
			return ERROR_NONSEQ_GRAMMER;
		} else if(nonseq_info->non_seq_check_ret == NONSEQ_KEY_WHILE) {
			nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WHILE;
			while(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] == 0 && this->nonseq_info->non_seq_struct_depth > 0) {
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth--]=0;
				if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_IF) {
					nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
					break;
				} else if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_DO) {
					nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
					break;
				}
			}
			if(nonseq_info->non_seq_struct_depth == 0 && nonseq_info->non_seq_type_stack[0] != NONSEQ_KEY_WAIT_WHILE && nonseq_info->non_seq_type_stack[0] != NONSEQ_KEY_WAIT_ELSE) {
				//save_sentence(str, len);
				nonseq_info->non_seq_exec = 1;
				return OK_NONSEQ_FINISH;
			} else {
				//save_sentence(str, len);
				return OK_NONSEQ_INPUTING;
			}
		}
	}
	if(nonseq_info->non_seq_check_ret) {
		nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 0;
		if(nonseq_info->non_seq_check_ret == NONSEQ_KEY_ELSE) {
			nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 2;
			if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] != NONSEQ_KEY_WAIT_ELSE || nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] != nonseq_info->brace_depth) {
				error("else is unmatch with if\n");
				nonseq_info->reset();
				return ERROR_NONSEQ_GRAMMER;
			}
			nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_IF;
		}
		nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = nonseq_info->non_seq_check_ret;
		nonseq_info->non_seq_struct_depth++;
		nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth = nonseq_info->non_seq_struct_depth;
	}
	if(len == 0)
		return OK_NONSEQ_INPUTING;
	if(nonseq_info->non_seq_struct_depth) {
		//save_sentence(str, len);
	}
	if(str[0] == '{') {
		nonseq_info->brace_depth++;
		if(nonseq_info->last_non_seq_check_ret) {
			this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] = nonseq_info->brace_depth;
		}
	} else if(str[0] == '}') {
		if(nonseq_info->brace_depth > 0) {
			if(nonseq_info->non_seq_struct_depth > 0 && this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] == nonseq_info->brace_depth) {
				do {
					nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth--]=0;
					if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_IF) {
						nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
						break;
					} else if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_DO) {
						nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
						break;
					}
				} while(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] == 0 && this->nonseq_info->non_seq_struct_depth > 0);
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 1;
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth = nonseq_info->non_seq_struct_depth + 1;
				if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_WAIT_ELSE) {
					//nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth++;
				}
				this->nonseq_info->nonseq_begin_stack_ptr--;
				if(nonseq_info->non_seq_struct_depth == 0)
					if(nonseq_info->non_seq_type_stack[0] == NONSEQ_KEY_WAIT_ELSE) {
					} else if(nonseq_info->non_seq_type_stack[0] == NONSEQ_KEY_WAIT_WHILE) {
					} else {
						nonseq_info->non_seq_exec = 1;
					}
			}
			nonseq_info->brace_depth--;
		} else {
			error("there is no { to match\n");
			return ERROR_NONSEQ_GRAMMER;
		}
	}
	if(nonseq_info->last_non_seq_check_ret && nonseq_info->non_seq_struct_depth && str[len-1] == ';') {// && nonseq_info->brace_depth == 0
		do {
			nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth--]=0;
			if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_IF) {
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
				break;
			} else if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_DO) {
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
				break;
			}
		} while(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] == 0 && this->nonseq_info->non_seq_struct_depth > 0);
		nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 1;
		nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth = nonseq_info->non_seq_struct_depth + 1;
		if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_WAIT_ELSE) {
			//nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth++;
		}
		if(nonseq_info->non_seq_struct_depth == 0) {
			if(nonseq_info->non_seq_type_stack[0] == NONSEQ_KEY_WAIT_ELSE) {
				return OK_NONSEQ_INPUTING;
			} else if(nonseq_info->non_seq_type_stack[0] == NONSEQ_KEY_WAIT_WHILE) {
				return OK_NONSEQ_INPUTING;
			} else {
				nonseq_info->non_seq_exec = 1;
				return OK_NONSEQ_FINISH;
			}
		}
	}
	return ERROR_NO;
}

int c_interpreter::function_analysis(char* str, uint len)
{
	int ret_function_define;
	if(this->function_flag_set.function_flag) {
		this->function_declare->save_sentence(str, len);
		if(this->function_flag_set.function_begin_flag) {
			if(str[0] != '{' || len != 1) {
				return ERROR_FUNC_ERROR;
			}
			this->function_flag_set.function_begin_flag = 0;
		}
		if(str[0] == '{') {
			this->function_flag_set.brace_depth++;
		} else if(str[0] == '}') {
			this->function_flag_set.brace_depth--;
			if(!this->function_flag_set.brace_depth) {
				this->function_flag_set.function_flag = 0;
				return OK_FUNC_FINISH;
			}
		}
		return OK_FUNC_INPUTING;
	}
	ret_function_define = optcmp(str);
	if(ret_function_define >= 0) {
		int l_bracket_pos = str_find(str, '(');
		int r_bracket_pos = str_find(str, ')');
		if(l_bracket_pos > 0 && r_bracket_pos > l_bracket_pos) {
			int symbol_begin_pos, ptr_level = 0;
			int keylen = strlen(type_key[ret_function_define]);
			stack* arg_stack;
			char function_name[32];
			this->function_flag_set.function_flag = 1;
			this->function_flag_set.function_begin_flag = 1;
			int i=keylen+1;
			symbol_begin_pos=(str[keylen]==' '?i:i-1);
			for(int j=symbol_begin_pos; str[j]=='*'; j++)
				ptr_level++;
			symbol_begin_pos += ptr_level;
			for(i=symbol_begin_pos; str[i]!='('; i++);
			memcpy(function_name, str + symbol_begin_pos, i - symbol_begin_pos);
			function_name[i - symbol_begin_pos] = 0;

			varity_attribute* arg_node_ptr = (varity_attribute*)vmalloc(sizeof(varity_attribute) * MAX_FUNCTION_ARGC);
			arg_stack = (stack*)vmalloc(sizeof(stack));
			arg_stack->init(sizeof(varity_attribute), arg_node_ptr, MAX_FUNCTION_ARGC);
			varity_attribute::init(arg_node_ptr, "", ret_function_define, 0, sizeof_type[ret_function_define]);
			arg_stack->push(arg_node_ptr++);
			bool void_flag = false;
			for(int i=l_bracket_pos+1; i<r_bracket_pos; i++) {
				char varity_name[32];
				int type, arg_name_begin_pos, arg_name_end_pos;
				int pos = key_match(str + i, r_bracket_pos-i+1, &type);
				if(pos < 0) {
					error("arg type error.\n");
					return ERROR_FUNC_ARG_LIST;					
				}
				keylen = strlen(type_key[type]);
				arg_name_begin_pos = i + pos + keylen + (str[i + pos + keylen] == ' ' ? 1 : 0);
				for(int j=arg_name_begin_pos; str[j]=='*'; j++)
					ptr_level++;
				if(type == VOID && !ptr_level) {
					if(arg_stack->get_count() > 1) {
						error("arg list error.\n");
						return ERROR_FUNC_ARG_LIST;
					}
					void_flag = true;
				} else {
					if(void_flag) {
						error("arg cannot use void type.\n");
						return ERROR_FUNC_ARG_LIST;
					}
				}
				arg_name_begin_pos += ptr_level;
				for(int j=arg_name_begin_pos; j<=r_bracket_pos; j++)
					if(str[j] == ',' || str[j] == ')') {
						arg_name_end_pos = j - 1;
						i = j;
						break;
					}
				memcpy(varity_name, str + arg_name_begin_pos, arg_name_end_pos - arg_name_begin_pos + 1);
				varity_name[arg_name_end_pos - arg_name_begin_pos + 1] = 0;
				if(!void_flag) {
					varity_attribute::init(arg_node_ptr, varity_name, type, 0, sizeof_type[type]);
					arg_stack->push(arg_node_ptr++);
				} else {
					if(varity_name[0] != 0) {
						error("arg cannot use void type.\n");
						return ERROR_FUNC_ARG_LIST;
					}
				}
			}
			//TODO: 释放申请的多余空间
			this->function_declare->declare(function_name, arg_stack);
			this->function_declare->save_sentence(str, len);
			return OK_FUNC_INPUTING;
		}
	}
	return OK_FUNC_NOFUNC;
}

#define RETURN(x) arg_string[0] = '('; \
	varity_global_flag = varity_global_flag_backup; \
	this->varity_declare->destroy_local_varity_cur_depth(); \
	this->varity_declare->local_varity_stack->dedeep(); \
	this->analysis_buf_ptr = analysis_buf_ptr_backup; \
	vfree(this->nonseq_info); \
	this->nonseq_info = nonseq_info_backup; \
	return x
int c_interpreter::call_func(char* name, char* arg_string, uint arg_len)
{
	int ret, arg_count;
	int varity_global_flag_backup = this->varity_global_flag;
	nonseq_info_struct* nonseq_info_backup = this->nonseq_info;
	char* analysis_buf_ptr_backup = this->analysis_buf_ptr;
	this->nonseq_info = (nonseq_info_struct*)vmalloc(sizeof(nonseq_info_struct));
	int arg_end_pos = arg_len - 1;
	varity_info arg_varity;
	this->varity_global_flag = VARITY_SCOPE_LOCAL;
	this->varity_declare->local_varity_stack->visible_depth = this->varity_declare->local_varity_stack->current_depth;
	this->varity_declare->local_varity_stack->endeep();
	arg_string[0] = 0;
	function_info* called_function_ptr = this->function_declare->find(name);
	this->analysis_buf_ptr = called_function_ptr->analysis_buf;
	arg_string[0] = ',';
	arg_count = called_function_ptr->arg_list->get_count();
	while(arg_count-- > 1) {
		varity_attribute* arg_ptr = (varity_attribute*)called_function_ptr->arg_list->visit_element_by_index(arg_count);
		this->varity_declare->declare(VARITY_SCOPE_LOCAL, arg_ptr->get_name(), arg_ptr->get_type(), arg_ptr->get_size());
		int comma_pos = str_find(arg_string, arg_end_pos, ',', 1);
		if(comma_pos >= 0) {
			this->sentence_exec(arg_string + comma_pos + 1, arg_end_pos - comma_pos - 1, false, &arg_varity);
			*(varity_info*)this->varity_declare->local_varity_stack->get_lastest_element() = arg_varity;
			arg_varity.reset();
		} else {
			error("Args insufficient\n");
			RETURN(ERROR_FUNC_ARGS);
		}
		arg_end_pos = comma_pos;
	}
	varity_attribute* arg_ptr = (varity_attribute*)called_function_ptr->arg_list->visit_element_by_index(0);
	this->function_return_value->init(arg_ptr, "", arg_ptr->get_type(), 0, arg_ptr->get_size());//TODO: check attribute.
	for(int row_line=2; row_line<called_function_ptr->row_line-1; row_line++) {
		ret = sentence_analysis(called_function_ptr->row_begin_pos[row_line], called_function_ptr->row_len[row_line]);
		if(ret < 0) {
			RETURN(ret);
		}
	}
	RETURN(ERROR_NO);
}
#undef RETURN(x)

int c_interpreter::sentence_analysis(char* str, uint len)
{
	int ret;
	ret = struct_analysis(str, len);
	if(ret != OK_STRUCT_NOSTRUCT)
		return ERROR_NO;
	ret = function_analysis(str, len);
	if(ret != OK_FUNC_NOFUNC)
		return ERROR_NO;
	ret = non_seq_struct_analysis(str, len);
	if(ret == OK_NONSEQ_FINISH)
		;//return ERROR_NO;
	else if(ret == ERROR_NONSEQ_GRAMMER)
		return ret;
	if(nonseq_info->non_seq_exec) {
		debug("exec non seq struct\n");
		nonseq_info->non_seq_exec = 0;
		ret = this->non_seq_section_exec(0, nonseq_info->row_num - 1);
		nonseq_info->reset();
		return ret;//avoid continue to exec single sentence.
	}
	if(!nonseq_info->non_seq_struct_depth && str[0] != '}') {
		ret = sentence_exec(str, len, true, NULL);
		return ret;
	}
	return ERROR_NO;
}

int c_interpreter::nesting_nonseq_section_exec(int line_begin, int line_end)
{
	int single_sentence_ret;
	int row_line;
	for(row_line=line_begin+1; row_line<=line_end; row_line++) {
		if(nonseq_info->row_info_node[row_line].non_seq_depth > nonseq_info->row_info_node[line_begin].non_seq_depth && nonseq_info->row_info_node[row_line].non_seq_info == 0) {
			int i;
			for(i=row_line+1; i<=line_end; i++) {
				if(nonseq_info->row_info_node[i].non_seq_depth < nonseq_info->row_info_node[row_line].non_seq_depth && nonseq_info->row_info_node[i].non_seq_depth || nonseq_info->row_info_node[i].non_seq_depth == nonseq_info->row_info_node[row_line].non_seq_depth && nonseq_info->row_info_node[i].non_seq_info == 1) {
					non_seq_section_exec(row_line, i);
					break;
				}
			}
			row_line = i;
			continue;
		}
		single_sentence_ret = this->sentence_exec(nonseq_info->row_info_node[row_line].row_ptr, nonseq_info->row_info_node[row_line].row_len, 1, 0);
		if(single_sentence_ret < 0)
			return single_sentence_ret;
	}
	return 0;
}

#define RETURN(x)	this->varity_global_flag = varity_global_flag_backup; \
	this->varity_declare->destroy_local_varity_cur_depth(); \
	this->varity_declare->local_varity_stack->dedeep(); \
	return x
int c_interpreter::non_seq_section_exec(int line_begin, int line_end)
{
	int varity_global_flag_backup = this->varity_global_flag;
	int row_line;
	int section_type;
	char* row_ptr;
	varity_info condition_varity;
	this->varity_global_flag = VARITY_SCOPE_LOCAL;
	this->varity_declare->local_varity_stack->endeep();
	row_line = line_begin;
	row_ptr = nonseq_info->row_info_node[row_line].row_ptr;
	section_type = non_seq_struct_check(row_ptr);
	varity_info::en_echo = 0;
	if(section_type == NONSEQ_KEY_FOR) {
		int l_bracket_pos = str_find(row_ptr, '(');
		int semi_pos_1st = str_find(row_ptr + l_bracket_pos + 1, ';');
		int semi_pos_2nd = str_find(row_ptr + l_bracket_pos + semi_pos_1st + 2, ';');
		int r_bracket_pos = str_find(row_ptr + l_bracket_pos + semi_pos_1st + semi_pos_2nd + 3, ')');
		int block_ret;
		this->sentence_exec(row_ptr + l_bracket_pos + 1, semi_pos_1st + 1, true, 0);
		while(1) {
			this->sentence_exec(row_ptr + l_bracket_pos + semi_pos_1st + 2, semi_pos_2nd, false, &condition_varity);
			int condition_type = condition_varity.get_type();
			if(condition_type == DOUBLE || condition_type == FLOAT){
				error("Float cannot become condtion\n");
				RETURN(ERROR_CONDITION_TYPE);
			}
			if(!condition_varity.is_non_zero())
				break;
			block_ret = nesting_nonseq_section_exec(line_begin, line_end);
			if(block_ret < 0) {
				error("for loop exec error!\n");
				RETURN(block_ret);
			}
			this->sentence_exec(row_ptr + l_bracket_pos + semi_pos_1st + semi_pos_2nd + 3, r_bracket_pos, false, 0);
		}
	} else if(section_type == NONSEQ_KEY_IF) {
		int l_bracket_pos = str_find(row_ptr, '(');
		int r_bracket_pos = str_find(row_ptr, ')', 1);
		int else_line;
		int condition_type;
		int block_ret;
		for(else_line=line_begin+1; else_line<line_end; else_line++)
			if(nonseq_info->row_info_node[else_line].non_seq_info == 2 && nonseq_info->row_info_node[else_line].non_seq_depth == nonseq_info->row_info_node[line_begin].non_seq_depth)
				break;
		this->sentence_exec(row_ptr + l_bracket_pos + 1, r_bracket_pos - l_bracket_pos - 1, false, &condition_varity);
		condition_type = condition_varity.get_type();
		if(condition_type == DOUBLE || condition_type == FLOAT){
			error("Float cannot become condtion\n");
			RETURN(ERROR_CONDITION_TYPE);
		}
		if(condition_varity.is_non_zero()) {
			debug("condition 1\n");
			if(else_line < line_end)
				block_ret = nesting_nonseq_section_exec(line_begin, else_line - 1);
			else
				block_ret = nesting_nonseq_section_exec(line_begin, line_end);
			if(block_ret < 0) {
				error("if block exec error!\n");
				RETURN(block_ret);
			}
		} else {
			if(else_line < line_end)
				block_ret = nesting_nonseq_section_exec(else_line, line_end);
		}
	} else if(section_type == NONSEQ_KEY_WHILE) {
		int l_bracket_pos = str_find(row_ptr, '(');
		int r_bracket_pos = str_find(row_ptr, ')', 1);
		int condition_type;
		int block_ret;
		while(1) {
			this->sentence_exec(row_ptr + l_bracket_pos + 1, r_bracket_pos - l_bracket_pos - 1, false, &condition_varity);
			condition_type = condition_varity.get_type();
			if(condition_type == DOUBLE || condition_type == FLOAT){
				error("Float cannot become condtion\n");
				RETURN(ERROR_CONDITION_TYPE);
			}
			if(!condition_varity.is_non_zero())
				break;
			debug("while condition 1\n");
			block_ret = nesting_nonseq_section_exec(line_begin, line_end);
			if(block_ret < 0) {
				error("while loop exec error!\n");
				RETURN(block_ret);
			}
		}
	}
	//}
	varity_info::en_echo = 1;
	RETURN(0);
}
#undef RETURN(x)

int c_interpreter::struct_analysis(char* str, uint len)
{
	int is_varity_declare = optcmp(str);
	if(this->struct_info_set.declare_flag) {
		if(this->struct_info_set.struct_begin_flag) {
			struct_info_set.struct_begin_flag = 0;
			if(str[0] != '{' || len != 1) {
				struct_info_set.declare_flag = 0;
				error("struct definition error\n");
				return ERROR_STRUCT_ERROR;
			} else {
				return OK_STRUCT_INPUTING;
			}
		} else {
			if(str[0] == '}') {
				struct_info_set.declare_flag = 0;
				this->struct_declare->current_node->struct_size = make_align(this->struct_info_set.current_offset, PLATFORM_WORD_LEN);
				return OK_STRUCT_FINISH;
				//重写reset，一次保留name，stack，二次全部reset。
				//vfree(this->struct_declare->current_node);
			} else {
				//this->struct_declare->save_sentence(str, len);
				char varity_name[32];
				stack *varity_stack_ptr = this->struct_declare->current_node->varity_stack_ptr;
				varity_info* new_node_ptr = (varity_info*)varity_stack_ptr->get_current_ptr();
				int varity_type, varity_name_begin_pos, ptr_level = 0, key_len;
				varity_type = optcmp(str);
				key_len = strlen(type_key[varity_type]);
				varity_name_begin_pos = key_len + (str[key_len] == ' ' ? 1 : 0); 
				int opt_len, opt_type, symbol_pos_once, symbol_pos_last = str[key_len]==' '?key_len+1:key_len, size = len - symbol_pos_last;
				int array_flag = 0, element_count = 1;
				ptr_level = 0;
				while(1) {
					for(; str[symbol_pos_last]=='*'; symbol_pos_last++)
						ptr_level++;
					size -= ptr_level;
					if((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) >= 0) {
						if(opt_type != OPT_COMMA && opt_type != OPT_EDGE && opt_type != OPT_L_MID_BRACKET && opt_type != OPT_R_MID_BRACKET) {
							error("Wrong operator exist in struct definition\n");
							return -1;
						} else if(opt_type == OPT_L_MID_BRACKET) {
							memcpy(varity_name, str + symbol_pos_last, symbol_pos_once);
							varity_name[symbol_pos_once] = 0;
							array_flag = 1;
						} else if(opt_type == OPT_R_MID_BRACKET) {
							array_flag = 2;
							element_count = y_atoi(str + symbol_pos_last, symbol_pos_once);
						} else {
							if(array_flag == 0) {
								memcpy(varity_name, str + symbol_pos_last, symbol_pos_once);
								varity_name[symbol_pos_once] = 0;
							} else if(array_flag == 2) {

							} else {
								
							}
							this->struct_info_set.current_offset = make_align(this->struct_info_set.current_offset, sizeof_type[varity_type]);
							new_node_ptr->arg_init(varity_name, varity_type, sizeof_type[varity_type] * element_count, (void*)this->struct_info_set.current_offset);
							this->struct_info_set.current_offset += sizeof_type[varity_type] * element_count;
							varity_stack_ptr->push();
							array_flag = 0;
							element_count = 1;
							ptr_level = 0;
						}
					} else {
						break;
					}
					symbol_pos_last += symbol_pos_once + opt_len;
					size -= symbol_pos_once + opt_len;
				}
				/*for(int j=varity_name_begin_pos; str[j]=='*'; j++)
					ptr_level++;
				varity_name_begin_pos += ptr_level;

				memcpy(varity_name, str + varity_name_begin_pos, len - varity_name_begin_pos + 1);
				varity_name[len - varity_name_begin_pos - 1] = 0;
				new_node_ptr->arg_init(varity_name, varity_type, sizeof_type[varity_type], (void*)this->struct_info_set.current_offset);
				this->struct_info_set.current_offset = make_align(this->struct_info_set.current_offset, sizeof_type[varity_type]) + sizeof_type[varity_type];
				varity_stack_ptr->push();*/

				return OK_STRUCT_INPUTING;
			}
		}
	}
	if(is_varity_declare == STRUCT) {
		int space_count = char_count(str + 6, ' ');
		if(space_count == 1) {//1:define 2+:declare
			int symbol_begin_pos, ptr_level = 0;
			int keylen = strlen(type_key[is_varity_declare]);
			stack* arg_stack;
			char struct_name[32];
			this->struct_info_set.declare_flag = 1;
			this->struct_info_set.struct_begin_flag = 1;
			this->struct_info_set.current_offset = 0;
			int i = keylen + 1;
			symbol_begin_pos = i;
			for(int j=symbol_begin_pos; str[j]=='*'; j++)
				ptr_level++;
			symbol_begin_pos += ptr_level;
			memcpy(struct_name, str + symbol_begin_pos, len - symbol_begin_pos);
			struct_name[len - symbol_begin_pos] = 0;
			varity_attribute* arg_node_ptr = (varity_attribute*)vmalloc(sizeof(varity_info) * MAX_VARITY_COUNT_IN_STRUCT);
			arg_stack = (stack*)vmalloc(sizeof(stack));
			arg_stack->init(sizeof(varity_info), arg_node_ptr, MAX_VARITY_COUNT_IN_STRUCT);
			this->struct_declare->declare(struct_name, arg_stack);
			return OK_STRUCT_INPUTING;
		}
	}
	return OK_STRUCT_NOSTRUCT;
}

int c_interpreter::key_word_analysis(char* str, uint len)
{
	int is_varity_declare;
	is_varity_declare = optcmp(str);
	char struct_name[32];
	struct_info* struct_node_ptr;
	if(is_varity_declare >= 0) {
		int key_len = strlen(type_key[is_varity_declare]);
		if(is_varity_declare != STRUCT)
			len = remove_char(str + key_len + 1, ' ') + key_len + 1;
		else {//TODO: 处理结构名后接*的情况
			int space_2nd_pos = str_find(str + key_len + 1, len - key_len - 1, ' ') + key_len + 1;
			if(str[space_2nd_pos - 1] == '*')
				space_2nd_pos--;
			memcpy(struct_name, str + key_len + 1, space_2nd_pos - key_len - 1);
			struct_name[space_2nd_pos - key_len - 1] = 0;
			struct_node_ptr = this->struct_declare->find(struct_name);
			if(!struct_node_ptr) {
				error("There is no struct called %s.\n", struct_name);
				return ERROR_STRUCT_NONEXIST;
			}
			len = remove_char(str + space_2nd_pos + 1, ' ') + space_2nd_pos + 1;
			key_len = space_2nd_pos;
		}
		/////////////////
		char varity_name[32];
		varity_info* new_varity_ptr;
		int array_flag = 0, element_count = 1, ptr_level = 0;
		int opt_len, opt_type, symbol_pos_once, symbol_pos_last = str[key_len]==' '?key_len+1:key_len, size = len - symbol_pos_last;
		while(size > 0) {
			for(; str[symbol_pos_last]=='*'; symbol_pos_last++)
				ptr_level++;
			size -= ptr_level;
			if((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) >= 0) {
				if(opt_type != OPT_COMMA && opt_type != OPT_EDGE && opt_type != OPT_L_MID_BRACKET && opt_type != OPT_R_MID_BRACKET && opt_type != OPT_ASSIGN) {
					error("Wrong operator exist in struct definition\n");
					return -1;
				} else if(opt_type == OPT_L_MID_BRACKET) {
					memcpy(varity_name, str + symbol_pos_last, symbol_pos_once);
					varity_name[symbol_pos_once] = 0;
					array_flag = 1;
				} else if(opt_type == OPT_R_MID_BRACKET) {
					array_flag = 2;
					element_count = y_atoi(str + symbol_pos_last, symbol_pos_once);
				} else {
					if(array_flag == 0) {
						memcpy(varity_name, str + symbol_pos_last, symbol_pos_once);
						varity_name[symbol_pos_once] = 0;
					} else if(array_flag == 2) {

					} else {
								
					}
					int ret;
					if(this->varity_global_flag == VARITY_SCOPE_GLOBAL) {
						if(ptr_level)
							ret = this->varity_declare->declare(VARITY_SCOPE_GLOBAL, varity_name, is_varity_declare + ptr_level * BASIC_VARITY_TYPE_COUNT, PLATFORM_WORD_LEN * element_count);
						else
							ret = this->varity_declare->declare(VARITY_SCOPE_GLOBAL, varity_name, is_varity_declare, sizeof_type[is_varity_declare] * element_count);
						new_varity_ptr = (varity_info*)this->varity_declare->global_varity_stack->get_lastest_element();
					} else {
						if(ptr_level)
							ret = this->varity_declare->declare(VARITY_SCOPE_LOCAL, varity_name, is_varity_declare + ptr_level * BASIC_VARITY_TYPE_COUNT, PLATFORM_WORD_LEN * element_count);
						else
							ret = this->varity_declare->declare(VARITY_SCOPE_LOCAL, varity_name, is_varity_declare, sizeof_type[is_varity_declare] * element_count);
						new_varity_ptr = (varity_info*)this->varity_declare->local_varity_stack->get_lastest_element();
					}
					if(ret)
						return ret;
					if(is_varity_declare == STRUCT) {
						new_varity_ptr->config_varity(0, struct_node_ptr);
						new_varity_ptr->struct_apply();
					}

					array_flag = 0;
					element_count = 1;
					ptr_level = 0;
					if(opt_type == OPT_ASSIGN) {
						int comma_pos;
						symbol_pos_last += symbol_pos_once + opt_len;
						for(comma_pos=symbol_pos_last; comma_pos<len; comma_pos++) {
							if(str[comma_pos] == ',' || str[comma_pos] == ';') {
								break;
							}
						}
						char assign_exec_buf[VARITY_ASSIGN_BUFLEN];
						char* analysis_buf_ptr_backup = this->analysis_buf_ptr;
						this->analysis_buf_ptr = assign_exec_buf;
						this->sentence_exec(str + symbol_pos_last, comma_pos - symbol_pos_last, false, new_varity_ptr);
						this->analysis_buf_ptr = analysis_buf_ptr_backup;
						size -= symbol_pos_once + opt_len + comma_pos - symbol_pos_last + 1;
						symbol_pos_last = comma_pos + 1;
						new_varity_ptr->echo();
						continue;
					}
				}
			} else {
				break;
			}
			symbol_pos_last += symbol_pos_once + opt_len;
			size -= symbol_pos_once + opt_len;
		}
		/////////////////
		/*for(uint i=keylen+1, symbol_begin_pos=(str[i-1]==' '?i:i-1); i<len; i++) {
			if(str[i] == ',' || str[i] == ';') {
				int ptr_level = 0;
				char varity_name[32];
				int count = 1;
				varity_info* new_varity_ptr;
				for(int j=symbol_begin_pos; str[j]=='*'; j++)
					ptr_level++;
				symbol_begin_pos += ptr_level;
				int symbol_end_pos = str_find(str + symbol_begin_pos, '[');
				if(symbol_end_pos >= 0) {
					int rbrace_pos = str_find(str + symbol_begin_pos, ']');
					memcpy(varity_name, str + symbol_begin_pos, symbol_end_pos);
					varity_name[symbol_end_pos] = 0;	
					count = y_atoi(str + symbol_begin_pos + symbol_end_pos + 1, rbrace_pos - symbol_end_pos - 1);
				} else {
					memcpy(varity_name, str + symbol_begin_pos, i - symbol_begin_pos);
					varity_name[i - symbol_begin_pos] = 0;	
				}
				int ret;
				if(this->varity_global_flag == VARITY_SCOPE_GLOBAL) {
					if(ptr_level)
						ret = this->varity_declare->declare(VARITY_SCOPE_GLOBAL, varity_name, is_varity_declare + ptr_level * BASIC_VARITY_TYPE_COUNT, PLATFORM_WORD_LEN * count);
					else
						ret = this->varity_declare->declare(VARITY_SCOPE_GLOBAL, varity_name, is_varity_declare, sizeof_type[is_varity_declare] * count);
					new_varity_ptr = (varity_info*)this->varity_declare->global_varity_stack->get_lastest_element();
				} else {
					if(ptr_level)
						ret = this->varity_declare->declare(VARITY_SCOPE_LOCAL, varity_name, is_varity_declare + ptr_level * BASIC_VARITY_TYPE_COUNT, PLATFORM_WORD_LEN * count);
					else
						ret = this->varity_declare->declare(VARITY_SCOPE_LOCAL, varity_name, is_varity_declare, sizeof_type[is_varity_declare] * count);
					new_varity_ptr = (varity_info*)this->varity_declare->local_varity_stack->get_lastest_element();
				}
				if(ret)
					return ret;
				if(is_varity_declare == STRUCT) {
					new_varity_ptr->config_varity(0, struct_node_ptr);
					new_varity_ptr->struct_apply();
				}
				symbol_begin_pos = i + 1;
			}
		}*/
		return OK_VARITY_DECLARE;
	}
	if(!strequ(str, "return ", 7)) {
		this->function_return_value->reset();
		this->sentence_exec(str + 7, len - 7, true, function_return_value);
		return OK_FUNC_RETURN;
	}
	return ERROR_NO;
}

int c_interpreter::sentence_exec(char* str, uint len, bool need_semicolon, varity_info* expression_value)
{
	int i;
	int total_bracket_depth;
	uint analysis_varity_count;
	char ch_last = str[len];
	int source_len = len;
	str[source_len] = 0;
	if(str[0] == '{' || str[0] == '}')
		return 0;
	if(str[len-1] != ';' && need_semicolon) {
		error("Missing ;\n");
		str[source_len] = ch_last;
		return ERROR_MISS_SEMICOLON;
	}
	strcpy(this->analysis_buf_ptr, str);
	analysis_varity_count = this->varity_declare->analysis_varity_stack->get_count();
	int key_word_ret = key_word_analysis(this->analysis_buf_ptr, len);
	if(key_word_ret) {
		str[source_len] = ch_last;
		return key_word_ret;
	}
	total_bracket_depth = get_bracket_depth(analysis_buf_ptr);
	if(total_bracket_depth < 0)
		return ERROR_BRACKET_UNMATCH;
	//从最深级循环解析深度递减的各级，以立即数/临时变量？表示各级返回结果
	char sub_analysis_buf[MAX_SUB_ANA_BUFLEN];
	for(i=total_bracket_depth; i>0; i--) {
		int current_depth = 0, sub_sentence_begin_pos, sub_sentence_end_pos;
		for(uint j=0; j<len; j++) {
			if(analysis_buf_ptr[j] == '(' || analysis_buf_ptr[j] == '[') {
				current_depth++;
				if(current_depth == i)
					sub_sentence_begin_pos = j;
			} else if(analysis_buf_ptr[j] == ')' || analysis_buf_ptr[j] == ']') {
				if(current_depth == i) {
					int opt_len, opt_type, symbol_begin_pos;//先检查是否是函数调用
					sub_sentence_end_pos = j;
					symbol_begin_pos = search_opt(analysis_buf_ptr, sub_sentence_begin_pos, 1, &opt_len, &opt_type) + opt_len;
					if(0 < symbol_begin_pos && symbol_begin_pos < sub_sentence_begin_pos && analysis_buf_ptr[symbol_begin_pos] != ' ' && analysis_buf_ptr[j] == ')') {
						char tmp_varity_name[3];
						varity_info *tmp_varity = 0;
						int ret = this->call_func(analysis_buf_ptr + symbol_begin_pos, analysis_buf_ptr + sub_sentence_begin_pos, sub_sentence_end_pos - sub_sentence_begin_pos + 1);
						this->varity_declare->declare_analysis_varity(0, 0, tmp_varity_name, &tmp_varity);
						if(this->function_return_value->get_content_ptr() == NULL && tmp_varity->get_type() != VOID) {
							error("Function has no return value\n");
							return ERROR_FUNC_RET_EMPTY;
						}
						if(ret == OK_FUNC_RETURN) {
							*tmp_varity = *this->function_return_value;//检查content非空，否则实际上没返回值。
							int delta_str_len = sub_replace(analysis_buf_ptr, symbol_begin_pos, sub_sentence_end_pos, tmp_varity_name);
							len += delta_str_len;
							j += delta_str_len;
						}
					} else {
						uint sub_sentence_length;
						sub_sentence_end_pos = j;
						sub_sentence_length = sub_sentence_end_pos - sub_sentence_begin_pos - 1;
						memcpy(sub_analysis_buf, analysis_buf_ptr + sub_sentence_begin_pos + 1, sub_sentence_length);
						sub_analysis_buf[sub_sentence_length] = 0;
						sub_sentence_analysis(sub_analysis_buf, &sub_sentence_length);
						if(analysis_buf_ptr[j] == ')') {
							sub_replace(analysis_buf_ptr, sub_sentence_begin_pos, sub_sentence_end_pos, sub_analysis_buf);
							len -= sub_sentence_end_pos - sub_sentence_begin_pos + 1 - sub_sentence_length;
							j -= sub_sentence_end_pos - sub_sentence_begin_pos + 1 - sub_sentence_length;
						} else {//TODO:中括号只算括号内，留待低级括号时作运算符处理，便于前述运算符的正确执行。
							int delta_len = sub_replace(analysis_buf_ptr, sub_sentence_begin_pos + 1, sub_sentence_end_pos - 1, sub_analysis_buf);
							len += delta_len;
							j += delta_len;
						}
					}
				}
				current_depth--;
			}
		}
	}
	if(need_semicolon)
		len -= 1;
	sub_sentence_analysis(analysis_buf_ptr, &len);
	if(expression_value != 0) {
		int expression_type = check_symbol(analysis_buf_ptr, len);
		if(expression_type == OPERAND_VARITY) {
			analysis_buf_ptr[len] = 0;
			varity_info *ret_varity_ptr = this->varity_declare->find(analysis_buf_ptr, PRODUCED_ALL);
			if(ret_varity_ptr == NULL) {
				str[source_len] = ch_last;
				return ERROR_VARITY_NONEXIST;
			}
			*expression_value = *ret_varity_ptr;
		} else if(expression_type == OPERAND_FLOAT) {
			*expression_value = y_atof(analysis_buf_ptr);
		} else if(expression_type == OPERAND_INTEGER) {
			*expression_value = y_atoi(analysis_buf_ptr);
		}
	}
	uint current_analysis_varity_count = this->varity_declare->analysis_varity_stack->get_count();
	this->varity_declare->destroy_analysis_varity(current_analysis_varity_count - analysis_varity_count);
	str[source_len] = ch_last;
	return ERROR_NO;
}

int c_interpreter::sub_sentence_analysis(char* str, uint *len)//无括号或仅含类型转换的子句解析
{
	int i;
	for(i=0; i<C_OPT_PRIO_COUNT; i++)
		if(this->c_opt_caculate_func_list[i]) {
			int ret = (this->*c_opt_caculate_func_list[i])(str,len);
			if(ret < 0)
				return ret;
		}
	return ERROR_NO;
}

int c_interpreter::non_seq_struct_check(char* str)
{
	return nonseq_key_cmp(str);
}
