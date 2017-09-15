#include "interpreter.h"

tty stdio;
c_interpreter myinterpreter(&stdio);

inline int IsSpace(char ch) {return (ch == ' ' || ch == '\t');}

int interpreter::call_func(char*, uint)
{
	return 0;
}

void interpreter::remove_extra_char(void)
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
	this->tty_used->puts(sentence_buf);
	this->tty_used->puts("\n");
}

int interpreter::run_interpreter(void)
{
	while(1) {
		tty_used->readline(sentence_buf);
		remove_extra_char();
		this->sentence_analysis(sentence_buf);
	}
}

c_interpreter::c_interpreter(terminal* tty_used)
{
	this->tty_used = tty_used;
}

int c_interpreter::sentence_analysis(char*)
{
	//先算括号栈深度
	//从最深级循环解析深度递减的各级，以立即数/临时变量？表示各级返回结果
	//各级解析时调用自身解析单句
	//运算符用循环调用15个函数分级解释
	return 0;
}
