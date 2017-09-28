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
	this->c_opt_caculate_func_list[3]=&c_interpreter::plus_opt;
	this->c_opt_caculate_func_list[5]=&c_interpreter::relational_opt;
	this->c_opt_caculate_func_list[13]=&c_interpreter::assign_opt;
}

int c_interpreter::save_sentence(char* str, uint len)
{
	analysis_info.row_info_node[analysis_info.row_num].row_ptr = &non_seq_tmp_buf[non_seq_code_fifo.wptr];
	analysis_info.row_info_node[analysis_info.row_num].row_len = len;
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
	}
	analysis_info.non_seq_check_ret = non_seq_struct_check(str);
	if(len && (analysis_info.non_seq_check_ret || analysis_info.non_seq_struct_depth)) {
		this->save_sentence(str, len);
		analysis_info.row_num++;
	}
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
						//save_sentence(str, len);
						analysis_info.non_seq_exec = 1;
						goto sentence_exec;
					} else {
						//save_sentence(str, len);
						return 0;
					}
				}
			} else { //analysis_info.non_seq_check_ret == NONSEQ_KEY_ELSE
				analysis_info.row_info_node[analysis_info.row_num - 2].non_seq_info = 3;
			}
		} else {
			if(analysis_info.non_seq_check_ret != NONSEQ_KEY_ELSE && len != 0) {
				while(this->analysis_info.nonseq_begin_bracket_stack[this->analysis_info.non_seq_struct_depth] == 0 && this->analysis_info.non_seq_struct_depth > 0) {
					//没清掉type_stack
					analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth--] = 0;
					if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_IF) {
						analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = 0;
						//analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
						//break;
					} else if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_DO) {
						analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
						break;
					}
				}
				analysis_info.row_info_node[analysis_info.row_num - 2].non_seq_depth = analysis_info.non_seq_struct_depth + 1;
				analysis_info.row_info_node[analysis_info.row_num - 2].non_seq_info = 1;
			} else if(analysis_info.non_seq_check_ret == NONSEQ_KEY_ELSE) {
				analysis_info.row_info_node[analysis_info.row_num - 2].non_seq_info = 3;
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
				//save_sentence(str, len);
				analysis_info.non_seq_exec = 1;
				goto sentence_exec;
			} else {
				//save_sentence(str, len);
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
		//save_sentence(str, len);
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
				analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_depth = analysis_info.non_seq_struct_depth + 1;
				if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_WAIT_ELSE) {
					//analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_depth++;
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
		analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_depth = analysis_info.non_seq_struct_depth + 1;
		if(analysis_info.non_seq_type_stack[analysis_info.non_seq_struct_depth] == NONSEQ_KEY_WAIT_ELSE) {
			//analysis_info.row_info_node[analysis_info.row_num - 1].non_seq_depth++;
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
		debug("exec non seq struct\n");
		this->non_seq_section_exec(0, analysis_info.row_num - 1);
		analysis_info.reset();
		return 0;//avoid continue to exec single sentence.
	}
	if(!analysis_info.non_seq_struct_depth && str[0] != '}') {
		sentence_exec(str, len, true, NULL);
	}
	return 0;
}

int c_interpreter::nesting_nonseq_section_exec(int line_begin, int line_end)
{
	int single_sentence_ret;
	int row_line;
	for(row_line=line_begin+1; row_line<=line_end; row_line++) {
		if(analysis_info.row_info_node[row_line].non_seq_depth > analysis_info.row_info_node[line_begin].non_seq_depth && analysis_info.row_info_node[row_line].non_seq_info == 0) {
			int i;
			for(i=row_line+1; i<=line_end; i++) {
				if(analysis_info.row_info_node[i].non_seq_depth < analysis_info.row_info_node[row_line].non_seq_depth || analysis_info.row_info_node[i].non_seq_depth == analysis_info.row_info_node[row_line].non_seq_depth && analysis_info.row_info_node[i].non_seq_info == 1) {
					non_seq_section_exec(row_line, i);
					break;
				}
			}
			row_line = i;
			continue;
		}
		single_sentence_ret = this->sentence_exec(analysis_info.row_info_node[row_line].row_ptr, analysis_info.row_info_node[row_line].row_len, 1, 0);
		if(single_sentence_ret)
			return single_sentence_ret;
	}
	return 0;
}

int c_interpreter::non_seq_section_exec(int line_begin, int line_end)
{
	int row_line, nesting_level = 0;
	int section_type;
	char* row_ptr;
	varity_info condition_varity;
	row_line = line_begin;
	row_ptr = analysis_info.row_info_node[row_line].row_ptr;
	section_type = non_seq_struct_check(row_ptr);
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
				debug("float cannot become condtion\n");
				return -1;
			}
			if(!condition_varity.is_non_zero())
				break;
			block_ret = nesting_nonseq_section_exec(line_begin, line_end);
			if(block_ret)
				return block_ret;
			this->sentence_exec(row_ptr + l_bracket_pos + semi_pos_1st + semi_pos_2nd + 3, r_bracket_pos, false, 0);
		}
	} else if(section_type == NONSEQ_KEY_IF) {
		int l_bracket_pos = str_find(row_ptr, '(');
		int r_bracket_pos = str_find(row_ptr, ')', 1);
		int else_line;
		int condition_type;
		int block_ret;
		for(else_line=line_begin+1; else_line<line_end; else_line++)
			if(analysis_info.row_info_node[else_line].non_seq_info == 2 && analysis_info.row_info_node[else_line].non_seq_depth == analysis_info.row_info_node[line_begin].non_seq_depth)
				break;
		this->sentence_exec(row_ptr + l_bracket_pos + 1, r_bracket_pos - l_bracket_pos - 1, false, &condition_varity);
		condition_type = condition_varity.get_type();
		if(condition_type == DOUBLE || condition_type == FLOAT){
			debug("float cannot become condtion\n");
			return -1;
		}
		if(condition_varity.is_non_zero()) {
			if(else_line < line_end)
				block_ret = nesting_nonseq_section_exec(line_begin, else_line - 1);
			else
				block_ret = nesting_nonseq_section_exec(line_begin, line_end);
			if(block_ret)
				return block_ret;
		} else {
			if(else_line < line_end)
				block_ret = nesting_nonseq_section_exec(else_line, line_end);
		}
	}
	//}
	return 0;
}

int c_interpreter::key_word_analysis(char* str, uint len)
{
	int is_varity_declare;
	is_varity_declare = optcmp(str);
	if(is_varity_declare >= 0) {
		int keylen = strlen(type_key[is_varity_declare]);
		int symbol_begin_pos;
		len = remove_char(str + keylen + 1, ' ') + keylen + 1;
		for(int i=keylen+1, symbol_begin_pos=(str[keylen]==' '?i:i-1); i<len; i++) {
			if(str[i] == ',' || str[i] == ';') {
				int ptr_level = 0;
				char varity_name[32];
				int count = 1;
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
				if(this->global_flag) {
					int ret;
					if(ptr_level)
						ret = this->varity_declare->declare(0, varity_name, is_varity_declare + ptr_level * 12, PLATFORM_WORD_LEN * count);
					else
						ret = this->varity_declare->declare(0, varity_name, is_varity_declare, sizeof_type[is_varity_declare] * count);
				} else {

				}
				symbol_begin_pos = i + 1;
			}
		}
		return 1;
	}
	return 0;
}

int c_interpreter::sentence_exec(char* str, uint len, bool need_semicolon, varity_info* expression_value)
{
	int i,j;
	int total_bracket_depth;
	char ch_last = str[len];
	int source_len = len;
	str[source_len] = 0;
	if(str[0] == '{' || str[0] == '}')
		return 0;
	if(str[len-1] != ';' && need_semicolon) {
		debug("Missing ;\n");
		str[source_len] = ch_last;
		return 1;
	}

	strcpy(this->analysis_buf, str);
	int key_word_ret = key_word_analysis(this->analysis_buf, len);
	if(key_word_ret) {
		str[source_len] = ch_last;
		return 0;
	}
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
					if(analysis_buf[j] == ')') {
						sub_replace(analysis_buf, sub_sentence_begin_pos, sub_sentence_end_pos, sub_analysis_buf);
						len -= sub_sentence_end_pos - sub_sentence_begin_pos + 1 - sub_sentence_length;
						j -= sub_sentence_end_pos - sub_sentence_begin_pos + 1 - sub_sentence_length;
					} else {

					}
				}
				current_depth--;
			}
		}
	}
	if(need_semicolon)
		len -= 1;
	sub_sentence_analysis(analysis_buf, &len);
	if(expression_value != 0) {
		varity_info* ret_varity_ptr = (varity_info*)varity_declare->analysis_varity_stack->pop();
		if(ret_varity_ptr == NULL)
			ret_varity_ptr = (varity_info*)this->varity_declare->find(analysis_buf, PRODUCED_DECLARE);
		if(ret_varity_ptr == NULL)
			return 1;
		*expression_value = *ret_varity_ptr;
	}
	this->varity_declare->destroy_analysis_varity();
	str[source_len] = ch_last;
	return 0;
}

int c_interpreter::sub_sentence_analysis(char* str, uint *len)//无括号或仅含类型转换的子句解析
{
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
