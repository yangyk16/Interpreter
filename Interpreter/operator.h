#pragma once
#ifndef OPERATOR_H
#define OPERATOR_H
#include "varity.h"

#define OPT_ASL_ASSIGN			0
#define OPT_ASR_ASSIGN			1
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
#define OPT_DOT					25
#define OPT_MINUS				26
#define OPT_BIT_REVERT			27
#define OPT_MUL					28
#define OPT_BIT_AND				29
#define OPT_NOT					30
#define OPT_DIVEDE				31
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

varity_info plus_opt(char*, uint);
varity_info assign_opt(char* str, uint size);

#endif