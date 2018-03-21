#pragma once
#ifndef OPERATOR_H
#define OPERATOR_H

#define HWREG(x) (*(volatile int*)(x))
#define CHAR_VALUE(x) (*(char*)(x))
#define U_CHAR_VALUE(x) (*(unsigned char*)(x))
#define SHORT_VALUE(x) (*(short*)(x))
#define U_SHORT_VALUE(x) (*(unsigned short*)(x))
#define INT_VALUE(x) (*(int*)(x))
#define U_INT_VALUE(x) (*(unsigned int*)(x))
#define LONG_VALUE(x) (*(long*)(x))
#define U_LONG_VALUE(x) (*(unsigned long*)(x))
#define LONG_LONG_VALUE(x) (*(long long*)(x))
#define U_LONG_LONG_VALUE(x) (*(unsigned long long*)(x))
#define FLOAT_VALUE(x) (*(float*)(x))
#define DOUBLE_VALUE(x) (*(double*)(x))
#define PTR_VALUE(x) (*(void**)(x))
#define PTR_N_VALUE(x) (*(unsigned long*)(x)) //long长度随平台变化，但16bit情况除外

#define OPERAND_VARITY		0
#define OPERAND_MEMBER		0
#define OPERAND_FLOAT		1
#define OPERAND_INTEGER		2

#define OPT_ASL_ASSIGN			0
#define OPT_ASR_ASSIGN			1
#define OPT_REFERENCE			2//no mid code
#define OPT_PLUS_PLUS			3//no mid code
#define OPT_MINUS_MINUS			4//no mid code
#define OPT_ASL					5
#define OPT_ASR					6
#define OPT_BIG_EQU				7
#define OPT_SMALL_EQU			8
#define OPT_EQU					9
#define OPT_NOT_EQU				10
#define	OPT_AND					11
#define OPT_OR					12
#define OPT_DEVIDE_ASSIGN		13
#define OPT_MUL_ASSIGN			14
#define OPT_MOD_ASSIGN			15
#define OPT_ADD_ASSIGN			16
#define OPT_MINUS_ASSIGN		17
#define OPT_BIT_AND_ASSIGN		18
#define OPT_BIT_XOR_ASSIGN		19
#define OPT_BIT_OR_ASSIGN		20
#define OPT_L_MID_BRACKET		21//no mid code
#define OPT_R_MID_BRACKET		22//no mid code
#define OPT_L_SMALL_BRACKET		23//no mid code
#define OPT_R_SMALL_BRACKET		24//no mid code
#define OPT_MEMBER				25
#define OPT_MINUS				26
#define OPT_BIT_REVERT			27
#define OPT_MUL					28
#define OPT_BIT_AND				29
#define OPT_NOT					30
#define OPT_DIVIDE				31
#define OPT_MOD					32
#define OPT_PLUS				33
#define OPT_BIG					34
#define OPT_SMALL				35
#define OPT_BIT_XOR				36
#define OPT_BIT_OR				37
#define OPT_TERNARY_Q			38//no mid code
#define OPT_TERNARY_C			39//no mid code
#define OPT_ASSIGN				40
#define OPT_COMMA				41//no mid code
#define OPT_EDGE				42//no mid code
#define OPT_NEGATIVE			43
#define OPT_POSITIVE			44
#define OPT_PTR_CONTENT			45
#define OPT_ADDRESS_OF			46
#define OPT_INDEX				47
#define OPT_CALL_FUNC			48
#define OPT_FUNC_COMMA 			49
#define OPT_L_PLUS_PLUS			50//no mid code
#define OPT_R_PLUS_PLUS			51//no mid code
#define OPT_L_MINUS_MINUS		52//no mid code
#define OPT_R_MINUS_MINUS		53//no mid code
#define OPT_TYPE_CONVERT		54
#define OPT_SIZEOF				55//no mid code
#define OPT_PASS_PARA			56

#define CTL_BRANCH				80
#define CTL_BRANCH_TRUE			81
#define CTL_BRANCH_FALSE		82
#define CTL_RETURN				84
#define CTL_BREAK				85
#define CTL_CONTINUE			86
#define CTL_GOTO				87
#define CTL_BXLR				88
#define CTL_SP_ADD				89

class c_interpreter;

int get_ret_type(int a, int b);
void handle_init(void);
int call_opt_handle(c_interpreter *interpreter_ptr);
int try_call_opt_handle(int opt, int opda_type, int opdb_type, int opda_complex_count, int *opda_type_info, int opdb_complex_count, int *opdb_type_info);

#endif
