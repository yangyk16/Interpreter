#include "interpreter.h"
#include <string.h>
#include <iostream>
#include "operator.h"
using namespace std;

tty stdio;
varity_info g_varity_node[MAX_G_VARITY_NODE];
varity_info l_varity_node[MAX_L_VARITY_NODE];
indexed_stack l_varity_list(sizeof(l_varity_node), l_varity_node);
stack g_varity_list(sizeof(g_varity_node), g_varity_node);
varity c_varity(&g_varity_list, &l_varity_list);
c_interpreter myinterpreter(&stdio, &c_varity, &c_varity);

char non_seq_key[][7] = {"if", "switch", "else", "for", "while", "do"};
char type_key[][10] = {"long long", "char", "short", "int", "long", "float", "double"};
const opt_calc c_opt_caculate_func_list[C_OPT_PRIO_COUNT] ={
	plus_opt
};

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
	for(i=0; i<7; i++) {
		for(j=0; type_key[i][j]; j++) {
			if(str[j] != type_key[i][j])
				break;
		}
		if(type_key[i][j] == 0)
			return i;
	}
	return -1;
}

char* trim(char* str)
{
	return 0;
}

char* remove_char(char* str, char ch)
{
	int rptr = 0, wptr = 0;
	while(str[rptr]) {
		if(str[rptr] != ch) {
			str[wptr++] = str[rptr++];
		}
	}
	str[wptr] = 0;
	return str;
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
	return 0;
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
		cout << this->sentence_analysis(sentence_buf, len);
	}
}

c_interpreter::c_interpreter(terminal* tty_used, varity* varity_declare, varity* temp_varity_analysis)
{
	this->tty_used = tty_used;
	this->varity_declare = varity_declare;
	this->temp_varity_analysis = temp_varity_analysis;
	this->opt_caculate_func_list = (opt_calc*)c_opt_caculate_func_list;
	this->row_pretreat_fifo.set_base(this->pretreat_buf);
	this->row_pretreat_fifo.set_length(sizeof(this->pretreat_buf));
	this->non_seq_code_fifo.set_base(this->non_seq_tmp_buf);
	this->non_seq_code_fifo.set_length(sizeof(this->non_seq_tmp_buf));
	this->nonseq_begin_stack_ptr = 0;
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
	int i;
	int total_bracket_depth;

	int is_varity_declare;
	is_varity_declare = keycmp(str);
	if(is_varity_declare >= 0) {
		int keylen = strlen(type_key[is_varity_declare]);
		if(str[keylen] == '*') {
			char* space_ptr = strchr(str, ' ');
			if(space_ptr != NULL) {
				int space_pos = space_ptr - str;
				str[keylen] = ' ';
				str[space_pos] = '*';
			}
		}

	}

	total_bracket_depth = get_bracket_depth(str);
	//从最深级循环解析深度递减的各级，以立即数/临时变量？表示各级返回结果
	for(i=total_bracket_depth; i>=0; i--) {

	}
	return 0;
}
//无括号或仅含类型转换的子句解析
int c_interpreter::sub_sentence_analysis(char* str, uint size)
{
	//运算符用循环调用15个函数分级解释
	return 0;
}

int c_interpreter::non_seq_struct_check(char* str)
{
	return 0;
}