#pragma once
#ifndef OPERATOR_H
#define OPERATOR_H

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

#define OPERAND_VARITY		0
#define OPERAND_MEMBER		0
#define OPERAND_FLOAT		1
#define OPERAND_INTEGER		2

#define OPT_ASL_ASSIGN			0
#define OPT_ASR_ASSIGN			1
#define OPT_REFERENCE			2
#define OPT_PLUS_PLUS			3
#define OPT_MINUS_MINUS			4
#define OPT_ASL					5
#define OPT_RSL					6
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
#define OPT_L_MID_BRACKET		21
#define OPT_R_MID_BRACKET		22
#define OPT_L_SMALL_BRACKET		23
#define OPT_R_SMALL_BRACKET		24
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
#define OPT_TERNARY_Q			38
#define OPT_TERNARY_C			39
#define OPT_ASSIGN				40
#define OPT_COMMA				41
#define OPT_EDGE				42
#define OPT_NEGATIVE			43
#define OPT_POSITIVE			44
#define OPT_PTR_CONTENT			45
#define OPT_ADDRESS_OF			46
#define OPT_INDEX				47
#define OPT_CALL_FUNC			48
#define OPT_FUNC_COMMA 			49
#define OPT_L_PLUS_PLUS			50
#define OPT_R_PLUS_PLUS			51
#define OPT_L_MINUS_MINUS		52
#define OPT_R_MINUS_MINUS		53

#define CTL_BRANCH				100
#define CTL_BRANCH_TRUE			101
#define CTL_BRANCH_FALSE		102
#define OPT_PASS_PARA			103

class mid_code;

int min(int a, int b);
void handle_init(void);
int call_opt_handle(mid_code*& instruction_ptr, char* sp, char *t_varity_sp);

#endif