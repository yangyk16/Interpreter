#pragma once
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "type.h"
#include "hal.h"
#include "config.h"
#include "varity.h"
#include "operator.h"

#define C_OPT_PRIO_COUNT		15
class c_interpreter;
//typedef varity_info (*opt_calc)(c_interpreter*,char*,uint);
typedef varity_info (__thiscall c_interpreter::*opt_calc )(char *,unsigned int);
class interpreter {
protected:
	varity* varity_declare;
	varity* temp_varity_analysis;//sentence_analysis produce mid-varity;
	char sentence_buf[MAX_SENTENCE_LENGTH];
	char analysis_buf[MAX_ANALYSIS_BUFLEN];
	terminal* tty_used;
	bool print_ret;

	int call_func(char*, uint);
	virtual int pre_treat(void){return 0;};
	virtual int sentence_analysis(char*, uint) = 0;
public:
	virtual int run_interpreter(void) = 0;
};

class c_interpreter: public interpreter {
	char pretreat_buf[MAX_PRETREAT_BUFLEN];
	round_queue row_pretreat_fifo;
	int brace_depth;
	int non_seq_struct_depth;
	char* non_seq_begin_addr[10];
	char non_seq_tmp_buf[NON_SEQ_TMPBUF_LEN];
	round_queue non_seq_code_fifo;
	char nonseq_begin_bracket_stack[MAX_STACK_INDEX];
	int nonseq_begin_stack_ptr;
	bool global_flag;

	int non_seq_struct_check(char* str);
	int sub_sentence_analysis(char*, uint size);
	int sentence_exec(char*, uint);
	virtual int sentence_analysis(char*, uint);
	virtual int pre_treat(void);

	varity_info assign_opt(char* str, uint len);
	varity_info plus_opt(char* str, uint size);
	opt_calc c_opt_caculate_func_list[C_OPT_PRIO_COUNT];
public:
	c_interpreter(terminal*, varity*, varity*);
	virtual int run_interpreter(void);
};

int remove_substring(char* str, int index1, int index2);
#endif
