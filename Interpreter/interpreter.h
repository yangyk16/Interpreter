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

#define OPERAND_G_VARITY	(0 << 0)
#define OPERAND_L_VARITY	(1 << 0)
#define OPERAND_T_VARITY	(2 << 0)
#define OPERAND_L_S_VARITY	(3 << 0)//递进深度局部变量
#define OPERAND_LINK_VARITY	(1 << 2)
#define OPERAND_CONST   	(1 << 3)


#define C_OPT_PRIO_COUNT		15
class c_interpreter;
//typedef varity_info (*opt_calc)(c_interpreter*,char*,uint);
typedef int (c_interpreter::*opt_calc )(char*, uint*);

typedef struct row_info_relative_nonseq_s {
	char* row_ptr;
	short row_len;
	char non_seq_depth;
	char non_seq_info;
	char nonseq_type;
	char post_info_a;
	short post_info_b;
	short post_info_c;
	char finish_flag;
	char reserve;
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
	int stack_frame_size;
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
	node *tree_root;
	int short_calc_stack[MAX_LOGIC_DEPTH];//TODO:保存地址的变量换成平台相关宏
	char short_depth;
	void* label_addr[MAX_LABEL_COUNT];
	char label_name[MAX_LABEL_COUNT][MAX_LABEL_NAME_LEN];
	int label_count;
} sentence_analysis_data_struct_t;

typedef struct call_func_info_s {
	function_info *function_ptr[MAX_FUNCTION_DEPTH];
	int cur_arg_number[MAX_FUNCTION_DEPTH];
	int arg_count[MAX_FUNCTION_DEPTH];
	int cur_stack_frame_size[MAX_FUNCTION_DEPTH];
	int offset[MAX_FUNCTION_DEPTH];
	int function_depth;
} call_func_info_t;

class mid_code {
public:
	char break_flag;
	char opda_varity_type;//操作数1变量类型
	char opdb_varity_type;//操作数2变量类型
	unsigned char ret_operator;    //执行的操作
	int ret_addr;         //结果地址
	int opda_addr;        //操作数1地址
	int double_space1;    //操作数1为double立即数时使用的空间
	int opdb_addr;
	int double_space2;
	int data;
	char opda_operand_type;//操作数1类型：变量/立即数
	char opdb_operand_type;//操作数2类型：变量/立即数
	char ret_operand_type;
	char ret_varity_type;
};

class interpreter {
protected:
	varity* varity_declare;
	varity* temp_varity_analysis;//sentence_analysis produce mid-varity;
	function* function_declare;
	char sentence_buf[MAX_SENTENCE_LENGTH];
	terminal* tty_used;
	bool print_ret;

	virtual int pre_treat(void){return 0;};
	virtual int sentence_analysis(char*, int) = 0;
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
	round_queue non_seq_code_fifo;
	int varity_global_flag;
	//int function_depth;
	unsigned int analysis_buf_inc_stack[MAX_FUNCTION_DEPTH];
	int save_sentence(char*, uint);
	int non_seq_struct_check(char* str);
	int function_analysis(char*, uint);
	int struct_analysis(char*, uint);
	int c_interpreter::struct_end(int struct_end_flag, bool &exec_flag_bak);
	int non_seq_struct_analysis(char*, uint);
	int sub_sentence_analysis(char*, uint* size);
	int key_word_analysis(char*, uint);
	int label_analysis(char*, int);
	int sentence_exec(char*, uint, bool, varity_info*);
	//////////////////////////////////////////////////////////////////////
	round_queue token_fifo;
	sentence_analysis_data_struct_t sentence_analysis_data_struct;
	stack mid_code_stack;
	stack mid_varity_stack;
	stack link_varity_stack;
	stack *cur_mid_code_stack_ptr;
	char *stack_pointer;
	char *tmp_varity_stack_pointer;
	char simulation_stack[STACK_SIZE];
	char tmp_varity_stack[TMP_VARITY_STACK_SIZE];
	bool exec_flag;
	call_func_info_t call_func_info;
	int get_token(char *str, node_attribute_t *info);
	int generate_arg_list(char *str, int count, stack &arg_list_ptr);
	int generate_compile_func(void);
	bool is_operator_convert(char *str, int &type, int &opt_len, int &prio);
	int generate_mid_code(char *str, uint len, bool need_semicolon);
	int list_stack_to_tree(node* tree_node, list_stack* post_order_stack);
	int exec_mid_code(mid_code *pc, uint count);
	int nonseq_start_gen_mid_code(char *str, uint len, int non_seq_type);
	int nonseq_mid_gen_mid_code(char *str, uint len);
	int nonseq_end_gen_mid_code(char *str, uint len);
	int tree_to_code(node *tree, stack *code_stack);
	int operator_pre_handle(stack *code_stack_ptr, node *opt_node_ptr);
	int operator_mid_handle(stack *code_stack_ptr, node *opt_node_ptr);
	int operator_post_handle(stack *code_stack_ptr, node *opt_node_ptr);
	int generate_expression_value(stack *code_stack_ptr, node_attribute_t *opt_node_ptr);
	friend int call_opt_handle(c_interpreter *interpreter_ptr);
	friend int opt_call_func_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr);
	friend int sys_stack_step_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr);
	int test(char *str, uint len);
	void print_code(void);
	//////////////////////////////////////////////////////////////////////
	virtual int sentence_analysis(char*, int);
	virtual int pre_treat(void);

public:
	mid_code *pc;
	int init(terminal*, varity*, nonseq_info_struct*, function*, struct_define*);
	//c_interpreter(terminal*, varity*, nonseq_info_struct*, function*, struct_define*);
	virtual int run_interpreter(void);
};

#endif
