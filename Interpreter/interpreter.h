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
#include "macro.h"
#include "data_struct.h"
#include "text.h"

#define EXPORT_FLAG_EXEC		1//code+varity+data+string
#define EXPORT_FLAG_LINK		2//name+varity+code+data+string

#define EXTRA_FLAG_DEBUG		4//name+varity+varity_type+source+code+data+string
#define EXTRA_FLAG_REF			8//name+varity+varity_type+source+code+data+string+struct

#define LINK_ADDR				0//for execute immediately
#define LINK_NUMBER				1//for output exe file
#define LINK_STRNO				2//for output o file

#define RTL_FLAG_DELAY			0
#define RTL_FLAG_IMMEDIATELY	1

#define SYMBOL_SAVE			0
#define SYMBOL_RESTORE			1

#define STOP_FLAG_STOP			1
#define STOP_FLAG_RUN			0

#define TOKEN_KEYWORD_TYPE		1
#define TOKEN_KEYWORD_CTL		2
#define TOKEN_ARG_LIST			3
#define TOKEN_KEYWORD_NONSEQ	4
#define TOKEN_OPERATOR			5
#define TOKEN_NAME				6
#define TOKEN_CONST_VALUE		7
#define TOKEN_STRING			8
#define TOKEN_SPECIFIER			9
#define TOKEN_OTHER				10
#define TOKEN_ERROR				11
#define TOKEN_NONEXIST			12

#define SPECIFIER_EXTERN	1
#define SPECIFIER_DELETE	2
#define SPECIFIER_TYPEDEF	3

#define NONSEQ_KEY_IF		1
#define NONSEQ_KEY_SWITCH	2
#define NONSEQ_KEY_ELSE		3
#define NONSEQ_KEY_FOR		4
#define NONSEQ_KEY_WHILE	5
#define NONSEQ_KEY_DO		6
#define NONSEQ_KEY_CASE		7
#define NONSEQ_KEY_DEFAULT	8
#define NONSEQ_KEY_WAIT_ELSE	9
#define NONSEQ_KEY_WAIT_WHILE	10
#define NONSEQ_KEY_BRACE	11

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
#define OPERAND_FUNCTION	(64)
#define OPERAND_NONE		(128)

#define EXEC_FLAG_TRUE	true
#define EXEC_FLAG_FALSE	false

#define C_OPT_PRIO_COUNT		15

#define BREAKPOINT_REAL		(1 << 0)
#define BREAKPOINT_STEP		(1 << 1)
#define BREAKPOINT_EN		(1 << 7)

#define PCODE_ECHO_BIT		0
#define PCODE_NARROW_BIT	1
#define PCODE_ROW_BIT		8
#define PCODE_BEGIN_BIT		16

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
	int non_seq_exec;//TODO: maybe can be removed.
	row_info_struct row_info_node[MAX_NON_SEQ_ROW];
	uint row_num;
	char* non_seq_begin_addr[MAX_STACK_INDEX];
	char nonseq_begin_bracket_stack[MAX_STACK_INDEX];
	char non_seq_type_stack[MAX_STACK_INDEX];
	//char nonseq_analyse_status;
	//int nonseq_begin_stack_ptr;
	//int stack_frame_size;
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
	char value_type;//优先级
	char data;
	char count;
	short pos;
	short reserve;
} node_attribute_t;

typedef struct language_element_space {
	indexed_stack l_varity_list;
	stack g_varity_list;
	varity c_varity;
	nonseq_info_struct nonseq_info_s;
	stack function_list;
	function c_function;
	stack struct_list;
	struct_define c_struct;
	stack macro_list;
	macro c_macro;
	arg_stack_stack arg_stack_list;
	stack typedef_list;
	typedefine c_typedef;
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

typedef struct preprocess_info_s {
	unsigned short ifdef_level;
	unsigned char ifdef_status[MAX_IFDEF_DEPTH];
} preprocess_info_t;

class mid_code {
public:
	long ret_addr;         //结果地址
	long opda_addr;        //操作数1地址，务必保持在操作数1的4个相关量的第一个位置 TODO:定义联合体，合并addr与doublespace方便位数不同的平台移植
	int double_space1;    //操作数1为double立即数时使用的空间
	unsigned char opda_varity_type;//操作数1变量类型
	unsigned char opda_operand_type;//操作数1类型：变量/立即数
	unsigned char ret_varity_type;
	unsigned char ret_operand_type;
	long opdb_addr;		//操作数2四个相关变量顺序要保持和操作数1一致，有代码依赖
	int double_space2;
	unsigned char opdb_varity_type;
	unsigned char opdb_operand_type;
	unsigned char ret_operator;    //执行的操作	
	char break_flag;
	int data;
};

typedef struct compile_info_s {
	unsigned int total_size;
	unsigned int unstored_size;
	unsigned short export_flag;
	unsigned short extra_flag;
	unsigned int sum32;
} compile_info_t;

typedef struct compile_function_info_s {
	unsigned int function_count;
	unsigned int alldata_size;
	unsigned int rowline_size;
} compile_function_info_t;

typedef struct compile_varity_info_s {
	unsigned int varity_count;
	unsigned int init_varity_count;
	unsigned int bss_size;
	unsigned int data_size;
	unsigned int varity_size;
	unsigned int type_size;
} compile_varity_info_t;

typedef struct compile_struct_info_s {
	unsigned int struct_count;
	unsigned int varity_size;
} compile_struct_info_t;

typedef struct compile_string_info_s {
	unsigned int string_count;
	unsigned int string_size;
	unsigned int name_count;
	unsigned int name_size;
} compile_string_info_t;

typedef struct interprete_need_s {
	stack mid_code_stack;
	stack mid_varity_stack;
	char pretreat_buf[MAX_PRETREAT_BUFLEN];
	char sentence_buf[MAX_SENTENCE_LENGTH];
	round_queue row_pretreat_fifo;
	round_queue non_seq_code_fifo;
	char non_seq_tmp_buf[NON_SEQ_TMPBUF_LEN];
	int str_count_bak;
	indentator indentation;
	sentence_analysis_data_struct_t sentence_analysis_data_struct;//TODO:移到interprete need
} interprete_need_t;

typedef struct cpsr_s {
	int last_type;
	PLATFORM_WORD *last_ret_addr;
} cpsr_t;

typedef struct mid_ret_s {
	PLATFORM_WORD *opda_addr;
	PLATFORM_WORD *opdb_addr;
	long long opda;
	long long opdb;
} mid_ret_t;

int user_eval(char *str);
int get_token(char* str, node_attribute_t* info);
extern "C" void global_init(void);
extern "C" void global_dispose(void);
extern "C" void run_interpreter(void);
extern "C" void stop_running(void);
void clear_arglist(stack *arg_stack_ptr);
void irq_reg(int irq_no, void *func_ptr, void *data);
int refscript(char *file);

class c_interpreter {
	static language_elment_space_t language_elment_space;
	static int cstdlib_func_count;
	int real_time_link;
	node *root;
	interprete_need_t *interprete_need_ptr;
	terminal* tty_used;
	terminal* tty_log;
	nonseq_info_struct* nonseq_info;
	struct_info_struct struct_info_set;
	function_flag_struct function_flag_set;
	macro* macro_declare;
	struct_define* struct_declare;
	varity* varity_declare;
	function* function_declare;
	typedefine* type_define;
	int varity_global_flag;
	int break_flag;
	
	char *stack_pointer;
	char *tmp_varity_stack_pointer;
	node_attribute_t *token_node_ptr;
	mid_code *pc;
	stack *cur_mid_code_stack_ptr;
	unsigned int simulation_stack_size;
	char *simulation_stack;
	char tmp_varity_stack[TMP_VARITY_STACK_SIZE];
	bool exec_flag;
	call_func_info_t call_func_info;
	mid_ret_t mid_ret;
	static preprocess_info_t preprocess_info;
	static compile_info_t compile_info;
	static compile_function_info_t compile_function_info;
	static compile_string_info_t compile_string_info;
	static compile_varity_info_t compile_varity_info;
	static compile_struct_info_t compile_struct_info;
	
	int save_sentence(char*, uint);
	int function_analysis(node_attribute_t*, int&);
	int function_reset(void);
	int struct_analysis(node_attribute_t*, uint);
	int get_nonseq_to_depth(int, bool);
	int struct_end(int struct_end_flag, bool &exec_flag_bak, bool try_flag);
	int non_seq_struct_analysis(node_attribute_t*, uint);
	//int non_seq_struct_analysis2(node_attribute_t*, uint);
	int varity_declare_analysis(node_attribute_t*, int);
	int label_analysis(node_attribute_t*, int);
	int sentence_exec(node_attribute_t*, uint, bool);
	int get_varity_type(node_attribute_t*, int&, char *name, int basic_type, struct_info *info, PLATFORM_WORD *&ret_info);
	int generate_arg_list(const char *str, int count, int &arg_count, PLATFORM_WORD *&arg);
	int generate_compile_func(void);
	int post_order_expression(node_attribute_t *node_ptr, int count, list_stack&);
	int generate_mid_code(node_attribute_t*, int count, bool need_semicolon);
	int get_expression_type(char *str, varity_info *&ret);
	int ulink(stack *stack_ptr, int mode);//link one function
	
	int mem_rearrange(void);
	int symbol_sr_status(int);
	int list_to_tree(node* tree_node, list_stack* post_order_stack);
	int ctl_analysis(node_attribute_t*, int);
	
	int nonseq_start_gen_mid_code(node_attribute_t*, int, int non_seq_type);
	int nonseq_mid_gen_mid_code(node_attribute_t*, int);
	int nonseq_end_gen_mid_code(int row_num, node_attribute_t*, int, int);
	int tree_to_code(node *tree, stack *code_stack);
	int operator_pre_handle(stack *code_stack_ptr, node *opt_node_ptr);
	int operator_mid_handle(stack *code_stack_ptr, node *opt_node_ptr);
	int operator_post_handle(stack *code_stack_ptr, node *opt_node_ptr);
	int generate_expression_value(stack *code_stack_ptr, node_attribute_t *opt_node_ptr);
	int generate_token_list(char *str, uint len);
	int token_convert(node_attribute_t *node_ptr, int &count);
	void print_code(mid_code *ptr, int n, int echo);
	int tip_wrong(int pos, char* str = 0);
	int basic_type_check(node_attribute_t*, int &count, struct_info *&struct_info_ptr);
	bool gdb_check(void);
	int find_fptr_by_code(mid_code *mid_code_ptr, function_info *&fptr, int *line_ptr = 0);
	int open_eval(char*, bool);
	int open_ref(char*);
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
	static int opt_exist_handle(c_interpreter *interpreter_ptr);
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
	static char* code_complete_callback(char*, int);
	int post_treat(void);
	int pre_treat(char*, uint);
	int macro_instead(char *str, int &len);
	int preprocess(char *str, int &len);
	int eval(node_attribute_t*, int);
	friend int user_eval(char *str);
	friend int refscript(char *file);
	friend int exec_invisible_type_convert(c_interpreter* interpreter_ptr, int*& opda_addr, int*& opdb_addr);
	friend class gdb;
public:
	cpsr_t cpsr;
	int tlink(int mode);//link all function
	int load_ofile(char *file, int flag, void **load_base, void **bss_base);
	int write_ofile(const char *file, int export_flag, int extra_flag);
	int exec_mid_code(mid_code *pc, uint count);
	void set_break_flag(int flag) {break_flag = flag;}
	int print_call_stack(void);
	void set_tty(terminal*, terminal*);
	void get_tty(terminal** ttyused, terminal** ttylog) {if(ttyused) *ttyused = this->tty_used; if(ttylog) *ttylog = this->tty_log;}
	int init(terminal*, int, int, int);
	int dispose(void);
	int run_interpreter(void);
	int run_main(int, int, char**);
	int run_thread(function_info*);
};
#endif
