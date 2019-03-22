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

#define TOKEN_KEYWORD_TYPE		1
#define TOKEN_KEYWORD_CTL		2
#define TOKEN_ARG_LIST			3
#define TOKEN_KEYWORD_NONSEQ	4
#define TOKEN_OPERATOR			5
#define TOKEN_NAME				6
#define TOKEN_CONST_VALUE		7
#define TOKEN_STRING			8
#define TOKEN_OTHER				9
#define TOKEN_ERROR				10
#define TOKEN_NONEXIST			11

#define NONSEQ_KEY_IF		1
#define NONSEQ_KEY_SWITCH	2
#define NONSEQ_KEY_ELSE		3
#define NONSEQ_KEY_FOR		4
#define NONSEQ_KEY_WHILE	5
#define NONSEQ_KEY_DO		6
#define NONSEQ_KEY_WAIT_ELSE	7
#define NONSEQ_KEY_WAIT_WHILE	8

#define CTL_KEY_BREAK		1
#define CTL_KEY_CONTINUE	2
#define CTL_KEY_GOTO		3
#define CTL_KEY_RETURN		4

#define OPERAND_G_VARITY	(1)
#define OPERAND_L_VARITY	(2)
#define OPERAND_T_VARITY	(4)
#define OPERAND_LINK_VARITY	(8)
#define OPERAND_STRING		(16)
#define OPERAND_CONST   	(32)

#define EXEC_FLAG_TRUE	true
#define EXEC_FLAG_FALSE	false

#define C_OPT_PRIO_COUNT		15

#define BREAKPOINT_REAL		(1 << 0)
#define BREAKPOINT_STEP		(1 << 1)
#define BREAKPOINT_EN		(1 << 7)

class c_interpreter;
//typedef varity_info (*opt_calc)(c_interpreter*,char*,uint);
typedef int (c_interpreter::*opt_calc )(char*, uint*);

typedef struct row_info_relative_nonseq_s {
	char* row_ptr;
#if DEBUG_EN
	mid_code *row_code_ptr;
#endif
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
	operand_value_t value;
	char node_type;
	char value_type;
	char data;
	char count;
	int reserve;
} node_attribute_t;

typedef struct language_element_space {
	varity_info g_varity_node[MAX_G_VARITY_NODE];
	varity_info l_varity_node[MAX_L_VARITY_NODE];
	indexed_stack l_varity_list;
	stack g_varity_list;
	varity c_varity;
	nonseq_info_struct nonseq_info_s;
	function_info function_node[MAX_FUNCTION_NODE];
	stack function_list;
	function c_function;
	struct_info struct_node[MAX_STRUCT_NODE];
	stack struct_list;
	struct_define c_struct;
	int init_done;
} language_elment_space_t;

typedef struct sentence_analysis_data_struct_s {
	node_attribute_t node_attribute[MAX_ANALYSIS_NODE];
	node node_struct[MAX_ANALYSIS_NODE];
	node_attribute_t last_token;
	PLATFORM_WORD short_calc_stack[MAX_LOGIC_DEPTH];
	char short_depth;
	char sizeof_depth;
	uint sizeof_code_count[MAX_SIZEOF_DEPTH];
	void *label_addr[MAX_LABEL_COUNT];
	char label_name[MAX_LABEL_COUNT][MAX_LABEL_NAME_LEN];
	int label_count;
	node *tree_root;
} sentence_analysis_data_struct_t;

typedef struct call_func_info_s {
	function_info *function_ptr[MAX_FUNCTION_DEPTH];
	int cur_arg_number[MAX_FUNCTION_DEPTH];
	int arg_count[MAX_FUNCTION_DEPTH];
	int cur_stack_frame_size[MAX_FUNCTION_DEPTH];
	int offset[MAX_FUNCTION_DEPTH];
	int function_depth;
	int para_offset;
} call_func_info_t;

class mid_code {
public:
	long ret_addr;         //结果地址
	long opda_addr;        //操作数1地址，务必保持在操作数1的4个相关量的第一个位置 TODO:定义联合体，合并addr与doublespace方便位数不同的平台移植
	int double_space1;    //操作数1为double立即数时使用的空间
	char opda_varity_type;//操作数1变量类型
	char opda_operand_type;//操作数1类型：变量/立即数
	char ret_varity_type;
	char ret_operand_type;
	long opdb_addr;		//操作数2四个相关变量顺序要保持和操作数1一致，有代码依赖
	int double_space2;
	char opdb_varity_type;
	char opdb_operand_type;
	unsigned char ret_operator;    //执行的操作	
	char break_flag;
	int data;
};

int user_eval(char *str);
extern "C" void global_init(void);
extern "C" void run_interpreter(void);
void clear_arglist(stack *arg_stack_ptr);

class c_interpreter {
	static language_elment_space_t language_elment_space;
	static varity_type_stack_t varity_type_stack;
	node *root;
	char sentence_buf[MAX_SENTENCE_LENGTH];
	terminal* tty_used;
	char pretreat_buf[MAX_PRETREAT_BUFLEN];
	round_queue row_pretreat_fifo;
	nonseq_info_struct* nonseq_info;
	struct_info_struct struct_info_set;
	function_flag_struct function_flag_set;
	struct_define* struct_declare;
	varity* varity_declare;
	function* function_declare;
	char non_seq_tmp_buf[NON_SEQ_TMPBUF_LEN];
	round_queue non_seq_code_fifo;
	int varity_global_flag;
	int break_flag;
	unsigned int analysis_buf_inc_stack[MAX_FUNCTION_DEPTH];
	sentence_analysis_data_struct_t sentence_analysis_data_struct;
	char *stack_pointer;
	char *tmp_varity_stack_pointer;
	node_attribute_t *token_node_ptr;
	mid_code *pc;
	stack mid_code_stack;
	stack mid_varity_stack;
	stack *cur_mid_code_stack_ptr;
	char simulation_stack[STACK_SIZE];
	char tmp_varity_stack[TMP_VARITY_STACK_SIZE];
	bool exec_flag;
	call_func_info_t call_func_info;
	int save_sentence(char*, uint);
	int function_analysis(node_attribute_t*, int);
	int struct_analysis(node_attribute_t*, uint);
	int struct_end(int struct_end_flag, bool &exec_flag_bak, bool try_flag);
	int non_seq_struct_analysis(node_attribute_t*, uint);
	int varity_declare_analysis(node_attribute_t*, int);
	int label_analysis(node_attribute_t*, int);
	int sentence_exec(node_attribute_t*, uint, bool);
	int get_varity_type(node_attribute_t*, int&, char *name, int basic_type, struct_info *info, PLATFORM_WORD *&ret_info);
	int generate_arg_list(char *str, int count, stack &arg_list_ptr);
	int generate_compile_func(void);
	int get_token(char *str, node_attribute_t *info);
	bool is_operator_convert(char *str, char &type, int &opt_len, char &prio);
	int post_order_expression(node_attribute_t *node_ptr, int count, list_stack&);
	int generate_mid_code(node_attribute_t*, int count, bool need_semicolon);
	int ulink(stack *stack_ptr);
	int list_to_tree(node* tree_node, list_stack* post_order_stack);
	int ctl_analysis(node_attribute_t*, int);
	int exec_mid_code(mid_code *pc, uint count);
	int nonseq_start_gen_mid_code(node_attribute_t*, int, int non_seq_type);
	int nonseq_mid_gen_mid_code(node_attribute_t*, int);
	int nonseq_end_gen_mid_code(int row_num, node_attribute_t*, int);
	int tree_to_code(node *tree, stack *code_stack);
	int operator_pre_handle(stack *code_stack_ptr, node *opt_node_ptr);
	int operator_mid_handle(stack *code_stack_ptr, node *opt_node_ptr);
	int operator_post_handle(stack *code_stack_ptr, node *opt_node_ptr);
	int generate_expression_value(stack *code_stack_ptr, node_attribute_t *opt_node_ptr);
	int generate_token_list(char *str, uint len);
	int token_convert(node_attribute_t *node_ptr, int &count);
	void print_code(mid_code *ptr, int n, int echo);
	int basic_type_check(node_attribute_t*, int &count, struct_info *&struct_info_ptr);
	bool gdb_check(void);
	int find_fptr_by_code(mid_code *mid_code_ptr, function_info *&fptr, int *line_ptr = 0);
	int open_eval(char*, bool);
	////////////////////////////opt handle////////////////////////
	static int opt_asl_handle(c_interpreter *interpreter_ptr);
	static int opt_asr_handle(c_interpreter *interpreter_ptr);
	static int opt_big_equ_handle(c_interpreter *interpreter_ptr);
	static int opt_small_equ_handle(c_interpreter *interpreter_ptr);
	static int opt_equ_handle(c_interpreter *interpreter_ptr);
	static int opt_not_equ_handle(c_interpreter *interpreter_ptr);
	static int opt_devide_assign_handle(c_interpreter *interpreter_ptr);
	static int opt_mul_assign_handle(c_interpreter *interpreter_ptr);
	static int opt_mod_assign_handle(c_interpreter *interpreter_ptr);
	static int opt_add_assign_handle(c_interpreter *interpreter_ptr);
	static int opt_minus_assign_handle(c_interpreter *interpreter_ptr);
	static int opt_bit_and_assign_handle(c_interpreter *interpreter_ptr);
	static int opt_xor_assign_handle(c_interpreter *interpreter_ptr);
	static int opt_bit_or_assign_handle(c_interpreter *interpreter_ptr);
	static int opt_and_handle(c_interpreter *interpreter_ptr);
	static int opt_or_handle(c_interpreter *interpreter_ptr);
	static int opt_member_handle(c_interpreter *interpreter_ptr);
	static int opt_minus_handle(c_interpreter *interpreter_ptr);
	static int opt_bit_revert_handle(c_interpreter *interpreter_ptr);
	static int opt_mul_handle(c_interpreter *interpreter_ptr);
	static int opt_bit_and_handle(c_interpreter *interpreter_ptr);
	static int opt_not_handle(c_interpreter *interpreter_ptr);
	static int opt_divide_handle(c_interpreter *interpreter_ptr);
	static int opt_mod_handle(c_interpreter *interpreter_ptr);
	static int opt_plus_handle(c_interpreter *interpreter_ptr);
	static int opt_big_handle(c_interpreter *interpreter_ptr);
	static int opt_small_handle(c_interpreter *interpreter_ptr);
	static int opt_bit_xor_handle(c_interpreter *interpreter_ptr);
	static int opt_bit_or_handle(c_interpreter *interpreter_ptr);
	static int opt_assign_handle(c_interpreter *interpreter_ptr);
	static int opt_negative_handle(c_interpreter *interpreter_ptr);
	static int opt_positive_handle(c_interpreter *interpreter_ptr);
	static int opt_ptr_content_handle(c_interpreter *interpreter_ptr);
	static int opt_address_of_handle(c_interpreter *interpreter_ptr);
	static int opt_index_handle(c_interpreter *interpreter_ptr);
	static int opt_call_func_handle(c_interpreter *interpreter_ptr);
	static int opt_func_comma_handle(c_interpreter *interpreter_ptr);
	static int ctl_branch_handle(c_interpreter *interpreter_ptr);
	static int ctl_branch_true_handle(c_interpreter *interpreter_ptr);
	static int ctl_branch_false_handle(c_interpreter *interpreter_ptr);
	static int opt_pass_para_handle(c_interpreter *interpreter_ptr);
	static int ctl_return_handle(c_interpreter *interpreter_ptr);
	static int ctl_bxlr_handle(c_interpreter *interpreter_ptr);
	static int ctl_sp_add_handle(c_interpreter *interpreter_ptr);
	static void handle_init(void);
	static int call_opt_handle(c_interpreter *interpreter_ptr);
	//////////////////////////////////////////////////////////////////////
	int post_treat(void);
	int pre_treat(uint);
	int eval(node_attribute_t*, int);
	friend int user_eval(char *str);
	friend class gdb;
public:
	void set_break_flag(int flag) {break_flag = flag;}
	int print_call_stack(void);
	int init(terminal*);
	int run_interpreter(void);
};
#endif
