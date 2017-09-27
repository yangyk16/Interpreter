#include "interpreter.h"
#include <string.h>
#include <stdio.h>
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
c_interpreter myinterpreter(&stdio, &c_varity);

void analysis_info_struct::reset(void)
{
	memset(this, 0, sizeof(analysis_info_struct));
}

int interpreter::call_func(char*, uint)
{
	return 0;
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
		len = this->row_pretreat_fifo.readline(sentence_buf);
		if(len > 0) {

		} else {
			tty_used->readline(sentence_buf);
			len = pre_treat();
		}
		this->sentence_analysis(sentence_buf, len);
	}
}

c_interpreter::c_interpreter(terminal* tty_used, varity* varity_declare)
{
	this->tty_used = tty_used;
	this->varity_declare = varity_declare;
	this->row_pretreat_fifo.set_base(this->pretreat_buf);
	this->row_pretreat_fifo.set_length(sizeof(this->pretreat_buf));
	this->non_seq_code_fifo.set_base(this->non_seq_tmp_buf);
	this->non_seq_code_fifo.set_length(sizeof(this->non_seq_tmp_buf));
	this->non_seq_code_fifo.set_element_size(1);
	this->analysis_info.nonseq_begin_stack_ptr = 0;
	this->global_flag = 1;
	this->c_opt_caculate_func_list[0]=&c_interpreter::plus_opt;
	this->c_opt_caculate_func_list[1]=&c_interpreter::assign_opt;
}

int c_interpreter::save_sentence(char* str, uint len)
{
	non_seq_code_fifo.write(str, len);
	non_seq_code_fifo.write("\n", 1);
	return 0;
}

int c_interpreter::sentence_analysis(char* str, uint len)
{
	analysis_info.last_non_seq_check_ret = analysis_info.non_seq_check_ret;
	if(len == 0) {
		if(analysis_info.non_seq_struct_depth && analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] != NONSEQ_KEY_WAIT_ELSE) {
			debug("blank shouldn't appear here.\n");
			analysis_info.reset();
			return -1;
		}
	} else {
		analysis_info.row_num++;
	}
	analysis_info.non_seq_check_ret = non_seq_struct_check(str);
	if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_WAIT_ELSE) {
		if(analysis_info.brace_depth == 0) {
			if(len != 0 && analysis_info.non_seq_check_ret != NONSEQ_KEY_ELSE) {
				debug("if is unmatch with else or blank\n");
				analysis_info.reset();
				return -1;
			} else if(len == 0) {
				while(this->analysis_info.nonseq_begin_bracket_stack[this->analysis_info.non_seq_struct_depth] == 0 && this->analysis_info.non_seq_struct_depth > 0) {
					analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth--]=0;
					if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_IF) {
						analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
						//break;
					} else if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_DO) {
						analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
						break;
					}
				}
				analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_depth = analysis_info.non_seq_struct_depth + 1;
				analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_info = 1;
				if(analysis_info.non_seq_struct_depth == 0) {
					if(analysis_info.non_seq_type_stack[0] != NONSEQ_KEY_WAIT_WHILE) {
						save_sentence(str, len);
						analysis_info.non_seq_exec = 1;
						goto sentence_exec;
					} else {
						save_sentence(str, len);
						return 0;
					}
				}
			}
		} else {
			if(analysis_info.non_seq_check_ret != NONSEQ_KEY_ELSE && len != 0) {
				while(this->analysis_info.nonseq_begin_bracket_stack[this->analysis_info.non_seq_struct_depth] == 0 && this->analysis_info.non_seq_struct_depth > 0) {
					//没清掉type_stack
					analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth--] = 0;
					if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_IF) {
						analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
						//break;
					} else if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_DO) {
						analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
						break;
					}
				}
				analysis_info.row_info_node[analysis_info.row_num - 2].non_seq_depth = analysis_info.non_seq_struct_depth + 1;
				analysis_info.row_info_node[analysis_info.row_num - 2].non_seq_info = 1;
			}
		}
	} else if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_WAIT_WHILE) {
		if(analysis_info.non_seq_check_ret != NONSEQ_KEY_WHILE && len != 0) {
			debug("do is unmatch with while\n");
			analysis_info.reset();
			return -1;
		} else if(analysis_info.non_seq_check_ret == NONSEQ_KEY_WHILE) {
			analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = NONSEQ_KEY_WHILE;
			while(this->analysis_info.nonseq_begin_bracket_stack[this->analysis_info.non_seq_struct_depth] == 0 && this->analysis_info.non_seq_struct_depth > 0) {
				analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth--]=0;
				if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_IF) {
					analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
					break;
				} else if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_DO) {
					analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
					break;
				}
			}
			if(analysis_info.non_seq_struct_depth == 0 && analysis_info.non_seq_type_stack[0] != NONSEQ_KEY_WAIT_WHILE && analysis_info.non_seq_type_stack[0] != NONSEQ_KEY_WAIT_ELSE) {
				save_sentence(str, len);
				analysis_info.non_seq_exec = 1;
				goto sentence_exec;
			} else {
				save_sentence(str, len);
				return 0;
			}
		}
	}
	if(analysis_info.non_seq_check_ret) {
		analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_info = 0;
		if(analysis_info.non_seq_check_ret == NONSEQ_KEY_ELSE) {
			analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_info = 2;
			if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] != NONSEQ_KEY_WAIT_ELSE || analysis_info.nonseq_begin_bracket_stack[this->analysis_info.non_seq_struct_depth] != analysis_info.brace_depth) {
				debug("else is unmatch with if\n");
				analysis_info.reset();
				return -1;
			}
			analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = NONSEQ_KEY_IF;
		}
		analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = analysis_info.non_seq_check_ret;
		analysis_info.non_seq_struct_depth++;
		analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_depth = analysis_info.non_seq_struct_depth;
	}
	if(len == 0)
		return 0;
	if(analysis_info.non_seq_struct_depth) {
		save_sentence(str, len);
	}
	if(str[0] == '{') {
		analysis_info.brace_depth++;
		if(analysis_info.last_non_seq_check_ret) {
			this->analysis_info.nonseq_begin_bracket_stack[this->analysis_info.non_seq_struct_depth] = analysis_info.brace_depth;
		}
	} else if(str[0] == '}') {
		if(analysis_info.brace_depth > 0) {
			if(analysis_info.non_seq_struct_depth > 0 && this->analysis_info.nonseq_begin_bracket_stack[this->analysis_info.non_seq_struct_depth] == analysis_info.brace_depth) {
				do {
					analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth--]=0;
					if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_IF) {
						analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
						break;
					} else if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_DO) {
						analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
						break;
					}
				} while(this->analysis_info.nonseq_begin_bracket_stack[this->analysis_info.non_seq_struct_depth] == 0 && this->analysis_info.non_seq_struct_depth > 0);
				analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_info = 1;
				analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_depth = analysis_info.non_seq_struct_depth;
				if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_WAIT_ELSE) {
					analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_depth++;
				}
				this->analysis_info.nonseq_begin_stack_ptr--;
				if(analysis_info.non_seq_struct_depth == 0)
					if(analysis_info.non_seq_type_stack[0] == NONSEQ_KEY_WAIT_ELSE) {
					} else if(analysis_info.non_seq_type_stack[0] == NONSEQ_KEY_WAIT_WHILE) {
					} else {
						analysis_info.non_seq_exec = 1;
					}
			}
			analysis_info.brace_depth--;
		} else {
			return -1;
		}
	}
	if(analysis_info.last_non_seq_check_ret && analysis_info.non_seq_struct_depth && str[len-1] == ';') {// && analysis_info.brace_depth == 0
		do {
			analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth--]=0;
			if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_IF) {
				analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
				break;
			} else if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_DO) {
				analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
				break;
			}
		} while(this->analysis_info.nonseq_begin_bracket_stack[this->analysis_info.non_seq_struct_depth] == 0 && this->analysis_info.non_seq_struct_depth > 0);
		analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_info = 1;
		analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_depth = analysis_info.non_seq_struct_depth;
		if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_WAIT_ELSE) {
			analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_depth++;
		}
		if(analysis_info.non_seq_struct_depth == 0) {
			if(analysis_info.non_seq_type_stack[0] == NONSEQ_KEY_WAIT_ELSE) {
				return 0;
			} else if(analysis_info.non_seq_type_stack[0] == NONSEQ_KEY_WAIT_WHILE) {
				return 0;
			} else {
				analysis_info.non_seq_exec = 1;
			}
		}
	}
sentence_exec:
	if(analysis_info.non_seq_exec) {
		analysis_info.reset();
		debug("exec non seq struct\n");
		this->non_seq_section_exec();
		return 0;//avoid continue to exec single sentence.
	}
	if(!analysis_info.non_seq_struct_depth && str[0] != '}') {
		sentence_exec(str, len);
	}
	return 0;
}

int c_interpreter::non_seq_section_exec(void)
{
	char* str_row;
	int str_row_len;
	int section_len = this->non_seq_code_fifo.get_count();
	while(1) {
		str_row = non_seq_code_fifo.readline(str_row_len);
		analysis_info.non_seq_check_ret = non_seq_struct_check(str_row);
		if(analysis_info.non_seq_check_ret == NONSEQ_KEY_FOR) {
			
		}
	}
}

int c_interpreter::sentence_exec(char* str, uint len)
{
	int i,j;
	int total_bracket_depth;

	if(str[len-1] != ';') {
		debug("Missing ;\n");
		return 1;
	}

	int is_varity_declare;
	is_varity_declare = optcmp(str);
	if(is_varity_declare >= 0) {
		int keylen = strlen(type_key[is_varity_declare]);
		int symbol_begin_pos;
		len = remove_char(str + keylen + 1, ' ') + keylen + 1;
		for(i=keylen+1, symbol_begin_pos=(str[keylen]==' '?i:i-1); i<len; i++) {
			if(str[i] == ',' || str[i] == ';') {
				int ptr_level = 0;
				char varity_name[32];
				for(j=symbol_begin_pos; str[j]=='*'; j++)
					ptr_level++;
				symbol_begin_pos += ptr_level;
				memcpy(varity_name, str + symbol_begin_pos, i - symbol_begin_pos);
				varity_name[i - symbol_begin_pos] = 0;
				
				if(this->global_flag) {
					int ret;
					if(ptr_level)
						ret = this->varity_declare->declare(0, varity_name, is_varity_declare + ptr_level * 12, PLATFORM_WORD_LEN);
					else
						ret = this->varity_declare->declare(0, varity_name, is_varity_declare, sizeof_type[is_varity_declare]);
				} else {

				}
				symbol_begin_pos = i + 1;
			}
		}
		return 0;
	}
	strcpy(this->analysis_buf, str);
	total_bracket_depth = get_bracket_depth(analysis_buf);
	if(total_bracket_depth < 0)
		return ERROR_BRACKET_UNMATCH;
	//从最深级循环解析深度递减的各级，以立即数/临时变量？表示各级返回结果
	char sub_analysis_buf[MAX_SUB_ANA_BUFLEN];
	for(i=total_bracket_depth; i>0; i--) {
		int current_depth = 0, sub_sentence_begin_pos, sub_sentence_end_pos;
		for(int j=0; j<len; j++) {
			if(analysis_buf[j] == '(' || analysis_buf[j] == '[') {
				current_depth++;
				if(current_depth == i)
					sub_sentence_begin_pos = j;
			} else if(analysis_buf[j] == ')' || analysis_buf[j] == ']') {
				if(current_depth == i) {
					//先检查是否是函数调用
					uint sub_sentence_length;
					sub_sentence_end_pos = j;
					sub_sentence_length = sub_sentence_end_pos - sub_sentence_begin_pos - 1;
					memcpy(sub_analysis_buf, analysis_buf + sub_sentence_begin_pos + 1, sub_sentence_length);
					sub_analysis_buf[sub_sentence_length] = 0;
					sub_sentence_analysis(sub_analysis_buf, &sub_sentence_length);
					sub_replace(analysis_buf, sub_sentence_begin_pos, sub_sentence_end_pos, sub_analysis_buf);
					len -= sub_sentence_end_pos - sub_sentence_begin_pos + 1 - sub_sentence_length;
					j -= sub_sentence_end_pos - sub_sentence_begin_pos + 1 - sub_sentence_length;
				}
				current_depth--;
			}
		}
	}
	//sub_sentence_analysis(analysis_buf, len-1);
	len -= 1;
	sub_sentence_analysis(analysis_buf, &len);
	this->varity_declare->destroy_analysis_varity();
	return 0;
}
//无括号或仅含类型转换的子句解析
int c_interpreter::sub_sentence_analysis(char* str, uint *len)
{
	//运算符用循环调用15个函数分级解释
	int i;
	for(i=0; i<C_OPT_PRIO_COUNT; i++)
		if(this->c_opt_caculate_func_list[i])
			(this->*c_opt_caculate_func_list[i])(str,len);
	return 0;
}

int c_interpreter::non_seq_struct_check(char* str)
{
	return nonseq_key_cmp(str);
}
