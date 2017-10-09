#pragma once
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "type.h"
#include "hal.h"
#include "config.h"
#include "varity.h"
#include "function.h"
#include "operator.h"

#define NONSEQ_KEY_IF		1
#define NONSEQ_KEY_SWITCH	2
#define NONSEQ_KEY_ELSE		3
#define NONSEQ_KEY_FOR		4
#define NONSEQ_KEY_WHILE	5
#define NONSEQ_KEY_DO		6
#define NONSEQ_KEY_WAIT_ELSE	7
#define NONSEQ_KEY_WAIT_WHILE	8

#define C_OPT_PRIO_COUNT		15
class c_interpreter;
//typedef varity_info (*opt_calc)(c_interpreter*,char*,uint);
typedef int (c_interpreter::*opt_calc )(char*, uint*);

typedef struct row_info_relative_nonseq_s {
	char* row_ptr;
	uint row_len;
	int non_seq_depth;
	int non_seq_info;
} row_info_struct;

typedef struct analysis_info_struct_s {
	int brace_depth;
	int non_seq_struct_depth;
	int non_seq_check_ret;
	int last_non_seq_check_ret;
	int non_seq_exec;
	row_info_struct row_info_node[MAX_NON_SEQ_ROW];
	uint row_num;
	char* non_seq_begin_addr[MAX_STACK_INDEX];
	char nonseq_begin_bracket_stack[MAX_STACK_INDEX];
	char non_seq_type_stack[MAX_STACK_INDEX];
	int nonseq_begin_stack_ptr;
	void reset(void);
} analysis_info_struct;

typedef struct function_flag_struct_s {
	int function_flag;
	int function_begin_flag;
	int brace_depth;
} function_flag_struct;

class interpreter {
protected:
	varity* varity_declare;
	varity* temp_varity_analysis;//sentence_analysis produce mid-varity;
	function* function_declare;
	char sentence_buf[MAX_SENTENCE_LENGTH];
	char analysis_buf[MAX_ANALYSIS_BUFLEN];
	terminal* tty_used;
	bool print_ret;

	virtual int call_func(char*, char*, uint) {return 0;};
	virtual int pre_treat(void){return 0;};
	virtual int sentence_analysis(char*, uint) = 0;
public:
	virtual int run_interpreter(void) = 0;
};

class c_interpreter: public interpreter {
	char pretreat_buf[MAX_PRETREAT_BUFLEN];
	round_queue row_pretreat_fifo;
	analysis_info_struct* analysis_info;
	function_flag_struct function_flag_set;
	char non_seq_tmp_buf[NON_SEQ_TMPBUF_LEN];
	char* analysis_buf_ptr;
	round_queue non_seq_code_fifo;
	int varity_global_flag;
	varity_info* function_return_value;
	int save_sentence(char*, uint);
	int non_seq_struct_check(char* str);
	int function_analysis(char*, uint);
	int non_seq_struct_analysis(char*, uint);
	int sub_sentence_analysis(char*, uint* size);
	int key_word_analysis(char*, uint);
	int sentence_exec(char*, uint, bool, varity_info*);
	int non_seq_section_exec(int, int);
	int nesting_nonseq_section_exec(int, int);
	virtual int call_func(char*, char*, uint);
	virtual int sentence_analysis(char*, uint);
	virtual int pre_treat(void);

	int assign_opt(char* str, uint* len);
	int plus_opt(char* str, uint* size);
	int multiply_opt(char* str, uint* size);
	int relational_opt(char* str, uint* size);
	int equal_opt(char* str, uint* size);
	int bracket_opt(char*, char*, char*, uint*);
	opt_calc c_opt_caculate_func_list[C_OPT_PRIO_COUNT];
public:
	c_interpreter(terminal*, varity*, analysis_info_struct*, function*);
	virtual int run_interpreter(void);
};

int remove_substring(char* str, int index1, int index2);
#endif
