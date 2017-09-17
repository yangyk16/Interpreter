#include "interpreter.h"
#include <string.h>
#include <iostream>
#include "operator.h"
using namespace std;

tty stdio;
c_interpreter myinterpreter(&stdio);
const opt_calc c_opt_caculate_func_list[C_OPT_PRIO_COUNT] ={
	plus_opt
};

inline int IsSpace(char ch) {return (ch == ' ' || ch == '\t');}
//need a bracket stack save outer bracket
inline int get_bracket_depth(char* str)
{
	int max_depth = 0;
	int current_depth = 0;//parentheses
	int current_depth_m = 0;//bracket
	int current_bracket = 0;//0:parenthese,1:bracket
	int last_bracket = 0;
	int len = strlen(str);
	for(int i=0; i<len; i++) {
		if(str[i] == '(') {
			last_bracket = current_bracket;
			current_bracket = 0;
			current_depth++;
			if(current_depth + current_depth_m > max_depth)
				max_depth = current_depth + current_depth_m;
		} else if(str[i] == ')') {
			if(current_bracket != 0)
				return -4;
			current_depth--;
			if(current_depth < 0)
				return -1;
			current_bracket = last_bracket;
		}
		if(str[i] == '[') {
			last_bracket = current_bracket;
			current_bracket = 1;
			current_depth_m++;
			if(current_depth + current_depth_m > max_depth)
				max_depth = current_depth + current_depth_m;
		} else if(str[i] == ']') {
			if(current_bracket != 1)
				return -5;
			current_depth_m--;
			if(current_depth_m < 0)
				return -2;
			current_bracket = last_bracket;
		}
	}
	if(current_depth != 0 || current_depth_m != 0)
		return -3;
	else
		return max_depth;
}

int interpreter::call_func(char*, uint)
{
	return 0;
}

void c_interpreter::remove_extra_char(void)
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
}

int interpreter::run_interpreter(void)
{
	while(1) {
		tty_used->readline(sentence_buf);
		remove_extra_char();
		cout << this->sentence_analysis(sentence_buf);
	}
}

c_interpreter::c_interpreter(terminal* tty_used)
{
	this->tty_used = tty_used;
	this->opt_caculate_func_list = (opt_calc*)c_opt_caculate_func_list;
}

int c_interpreter::sentence_analysis(char* str)
{
	int total_bracket_depth;
	int i;
	total_bracket_depth = get_bracket_depth(str);
	//从最深级循环解析深度递减的各级，以立即数/临时变量？表示各级返回结果
	for(i=total_bracket_depth; i>=0; i--) {

	}

	return total_bracket_depth;
}

//无括号或仅含类型转换的子句解析
int c_interpreter::sub_sentence_analysis(char* str, uint size)
{
	//运算符用循环调用15个函数分级解释
	return 0;
}