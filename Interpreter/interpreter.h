#pragma once
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "type.h"
#include "hal.h"
#include "config.h"
#include "varity.h"
#include "function.h"
#include "struct.h"
#include "operator.h"
#include "data_struct.h"

#define NONSEQ_KEY_IF		1
#define NONSEQ_KEY_SWITCH	2
#define NONSEQ_KEY_ELSE		3
#define NONSEQ_KEY_FOR		4
#define NONSEQ_KEY_WHILE	5
#define NONSEQ_KEY_DO		6
#define NONSEQ_KEY_WAIT_ELSE	7
#define NONSEQ_KEY_WAIT_WHILE	8

#define OPERAND_G_VARITY	0
#define OPERAND_L_VARITY	1
#define OPERAND_A_VARITY	2
#define OPERAND_CONST   	3

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

typedef struct nonseq_info_struct_s {
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
} nonseq_info_struct;

typedef struct function_flag_struct_s {
	int function_flag;
	int function_begin_flag;
	int brace_depth;
} function_flag_struct;

typedef struct struct_info_s {
	int declare_flag;
	int struct_begin_flag;
	int current_offset;
} struct_info_struct;

typedef union operand_value {
	double double_value;
	int int_value;
	long long long_long_value;
	char* ptr_value;
} operand_value_t;

typedef struct node_attribute_s {
	int node_type;
	int value_type;
	operand_value_t value;
} node_attribute_t;

typedef struct sentence_analysis_data_struct_s {
	list_stack expression_tmp_stack;
	list_stack expression_final_stack;
	node_attribute_t node_attribute[MAX_ANALYSIS_NODE];
	node node_struct[MAX_ANALYSIS_NODE];
	node_attribute_t last_token;
} sentence_analysis_data_struct_t;

class mid_code {
public:
	char break_flag;
	char opda_varity_type;
	char opdb_varity_type;
	char reserve;
	int ret_addr;
	int opda_addr;
	int opdb_addr;
	char ret_operand_type;
	char opda_operand_type;
	char opdb_operand_type;
	char reserve;
	int exec_code(void);
};

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
	nonseq_info_struct* nonseq_info;
	struct_info_struct struct_info_set;
	function_flag_struct function_flag_set;
	struct_define* struct_declare;
	char non_seq_tmp_buf[NON_SEQ_TMPBUF_LEN];
	char* analysis_buf_ptr;
	round_queue non_seq_code_fifo;
	int varity_global_flag;
	varity_info* function_return_value;
	int function_depth;
	unsigned int analysis_buf_inc_stack[MAX_FUNCTION_DEPTH];
	int save_sentence(char*, uint);
	int non_seq_struct_check(char* str);
	int function_analysis(char*, uint);
	int struct_analysis(char*, uint);
	int non_seq_struct_analysis(char*, uint);
	int sub_sentence_analysis(char*, uint* size);
	int key_word_analysis(char*, uint);
	int sentence_exec(char*, uint, bool, varity_info*);
	int non_seq_section_exec(int, int);
	int nesting_nonseq_section_exec(int, int);
	//////////////////////////////////////////////////////////////////////
	round_queue token_fifo;
	sentence_analysis_data_struct_t sentence_analysis_data_struct;
	stack mid_code_stack;
	stack mid_varity_stack;
	int get_token(char *str, node_attribute_t *info);
	bool is_operator_convert(char *str, int &type, int &opt_len, int &prio);
	int construct_expression_tree(char *str, uint len);
	int tree_to_code(node *tree, stack *code_stack);
	int test(char *str, uint len);
	//////////////////////////////////////////////////////////////////////
	virtual int call_func(char*, char*, uint);
	virtual int sentence_analysis(char*, uint);
	virtual int pre_treat(void);

	int auto_inc_opt(char* str, uint* size_ptr);
	int assign_opt(char* str, uint* len);
	int plus_opt(char* str, uint* size_ptr);
	int multiply_opt(char* str, uint* size_ptr);
	int relational_opt(char* str, uint* size_ptr);
	int equal_opt(char* str, uint* size_ptr);
	int member_opt(char* str, uint* size_ptr);
	//int bracket_opt(char*, char*, char*, uint*);
	opt_calc c_opt_caculate_func_list[C_OPT_PRIO_COUNT];
public:
	c_interpreter(terminal*, varity*, nonseq_info_struct*, function*, struct_define*);
	virtual int run_interpreter(void);
};

int remove_substring(char* str, int index1, int index2);
#endif
