#include "interpreter.h"
#include <string.h>
#include <iostream>
#include "operator.h"
using namespace std;

tty stdio;
varity_info g_varity_node[MAX_G_VARITY_NODE];
varity_info l_varity_node[MAX_L_VARITY_NODE];
varity_info a_varity_node[MAX_A_VARITY_NODE];
stack a_varity_list(sizeof(varity_info), a_varity_node, MAX_A_VARITY_NODE);
indexed_stack l_varity_list(sizeof(varity_info), l_varity_node, MAX_L_VARITY_NODE);
stack g_varity_list(sizeof(varity_info), g_varity_node, MAX_G_VARITY_NODE);
varity c_varity(&g_varity_list, &l_varity_list, &a_varity_list);
c_interpreter myinterpreter(&stdio, &c_varity);

char non_seq_key[][7] = {"if", "switch", "else", "for", "while", "do"};
const char opt_str[][4] = {"[","]","(",")",".","->","-","~","++","--","*","&","!","/","%","+","<<",">>",">",">=","<","<=","==","!=","&","^","|","&&","||","?",":","=","/=","*=","%=","+=","-=","<<=",">>=","&=","^=","|=",","};

inline int IsSpace(char ch) {return (ch == ' ' || ch == '\t');}
//need a bracket stack save outer bracket
inline int get_bracket_depth(char* str)
{
	char bracket_type_stack[MAX_BRACKET_DEPTH];
	int max_depth = 0;
	int current_depth = 0;//parentheses
	int current_depth_m = 0;//bracket
	int len = strlen(str);

	for(int i=0; i<len; i++) {
		if(str[i] == '(') {
			bracket_type_stack[current_depth + current_depth_m] = 0;
			current_depth++;
			if(current_depth + current_depth_m > max_depth)
				max_depth = current_depth + current_depth_m;
		} else if(str[i] == ')') {
			current_depth--;
			if(bracket_type_stack[current_depth + current_depth_m] != 0)
				return -4;
			if(current_depth < 0)
				return -1;
		}
		if(str[i] == '[') {
			bracket_type_stack[current_depth + current_depth_m] = 1;
			current_depth_m++;
			if(current_depth + current_depth_m > max_depth)
				max_depth = current_depth + current_depth_m;
		} else if(str[i] == ']') {
			current_depth_m--;
			if(bracket_type_stack[current_depth + current_depth_m] != 1)
				return -5;
			if(current_depth_m < 0)
				return -2;
		}
	}
	if(current_depth != 0 || current_depth_m != 0)
		return -3;
	else
		return max_depth;
}

int keycmp(char* str)
{
	int i,j;
	for(i=0; i<sizeof(type_key)/sizeof(type_key[0]); i++) {
		for(j=0; type_key[i][j]; j++) {
			if(str[j] != type_key[i][j])
				break;
		}
		if(type_key[i][j] == 0 && (str[j] == ' ' || str[j] == '*'))
			return i;
	}
	return -1;
}

char* trim(char* str)
{
	return 0;
}

int remove_char(char* str, char ch)
{
	int rptr = 0, wptr = 0;
	while(str[rptr]) {
		if(str[rptr] != ch) {
			str[wptr++] = str[rptr];
		}
		rptr++;
	}
	str[wptr] = 0;
	return wptr;
}

int remove_substring(char* str, int index1, int index2)
{
	if(index2 < index1)
		return -1;
	int wptr = index1, rptr = index2 + 1;
	for(; str[rptr]; rptr++) {
		str[wptr++] = str[rptr];
	}
	str[wptr] = 0;
	return index2 - index1 + 1;
}

int search_opt(char* str, int size, int direction)
{
	int i, j;
	if(direction) {
		//direction=1: left->right; direction=0: right->left;
		//merge 2 cycles to 1 can save space, but not merge is easily to read.
		for(i=0; i<size; i++) {
			for(j=0; j<sizeof(opt_str)/sizeof(*opt_str);j++) {
				if(str[i] == opt_str[j][0]) {
					if(i < size - 1 && str[i+1] == opt_str[j][1]) {
						if(i < size - 2 && str[i+2] == opt_str[j][2]) {
							return i;
						}
						return i;
					}
					return i;
				}
			}
		}
	} else {
		for(i=size-1; i>=0; i--)
			for(j=0; j<sizeof(opt_str)/sizeof(*opt_str);j++) {
				if(str[i] == opt_str[j][0]) {
					if(i < size - 1 && str[i+1] == opt_str[j][1]) {
						if(i < size - 2 && str[i+2] == opt_str[j][2]) {
							return i;
						}
						return i;
					}
					return i;
				}
			}
	}
	return -1;
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
	this->nonseq_begin_stack_ptr = 0;
	this->global_flag = true;
	this->c_opt_caculate_func_list[0]=&c_interpreter::plus_opt;
	this->c_opt_caculate_func_list[1]=&c_interpreter::assign_opt;
}

int c_interpreter::sentence_analysis(char* str, uint len)
{
	int non_seq_check_ret, non_seq_exec = 0;
	
	non_seq_check_ret = non_seq_struct_check(str);
	if(non_seq_check_ret)
		non_seq_struct_depth++;
	if(non_seq_struct_depth) {
		non_seq_code_fifo.write(str, len);
		non_seq_code_fifo.write("\n", 1);
	}
	if(str[0] == '{') {
		brace_depth++;
		if(non_seq_check_ret) {
			this->nonseq_begin_bracket_stack[this->nonseq_begin_stack_ptr++] = brace_depth;
		}
		varity_declare->local_varity_stack->endeep();
	} else if(str[0] == '}') {
		if(brace_depth > 0) {
			if(non_seq_struct_depth > 0 && this->nonseq_begin_bracket_stack[this->nonseq_begin_stack_ptr] == brace_depth) {
				non_seq_struct_depth--;
				this->nonseq_begin_stack_ptr--;
				if(non_seq_struct_depth == 0)
					non_seq_exec = 1;
			}
			brace_depth--;
			varity_declare->local_varity_stack->dedeep();
		} else {
			return -1;
		}
	}
	if(non_seq_struct_depth && brace_depth == 0 && str[len-1] == ';') {
		non_seq_struct_depth--;
		if(non_seq_struct_depth == 0)
			non_seq_exec = 1;
	}
	if(non_seq_exec) {

	}
	if(!non_seq_struct_depth && str[0] != '}') {
		sentence_exec(str, len);
	}
	return 0;
}

int c_interpreter::sentence_exec(char* str, uint len)
{
	int i,j;
	int total_bracket_depth;

	if(str[len-1] != ';') {
		cout << "Missing ;" << endl;
		return 1;
	}

	int is_varity_declare;
	is_varity_declare = keycmp(str);
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
						ret = this->varity_declare->declare(this->global_flag, varity_name, is_varity_declare + ptr_level * 12, PLATFORM_WORD_LEN);
					else
						ret = this->varity_declare->declare(this->global_flag, varity_name, is_varity_declare, sizeof_type[is_varity_declare]);
					if(ret)
						cout << "declare varity \"" << varity_name << "\" error: " << ret << endl;
				} else {

				}
				symbol_begin_pos = i + 1;
			}
		}
		return 0;
	}
	strcpy(this->analysis_buf, str);
	total_bracket_depth = get_bracket_depth(str);
	//从最深级循环解析深度递减的各级，以立即数/临时变量？表示各级返回结果
	for(i=total_bracket_depth; i>=0; i--) {
		
	}
	//this->assign_opt(str,len-1);
	sub_sentence_analysis(analysis_buf, len-1);
	return 0;
}
//无括号或仅含类型转换的子句解析
int c_interpreter::sub_sentence_analysis(char* str, uint len)
{
	//运算符用循环调用15个函数分级解释
	this->assign_opt(str,len);
	return 0;
}

int c_interpreter::non_seq_struct_check(char* str)
{
	return 0;
}