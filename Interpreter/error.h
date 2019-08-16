#pragma once
#ifndef ERROR_H
#define ERROR_H

#define ERROR_NO				0
#define ERROR_BRACKET_UNMATCH	-1
#define ERROR_VARITY_DUPLICATE	-2
#define ERROR_VARITY_COUNT_MAX	-3
#define ERROR_VARITY_NOSPACE	-4
#define ERROR_NONSEQ_GRAMMER	-5
#define ERROR_FUNC_ERROR    	-6
#define ERROR_FUNC_DUPLICATE	-7
#define ERROR_FUNC_COUNT_MAX	-8
#define ERROR_FUNC_ARGS			-9
#define ERROR_FUNC_RET_EMPTY	-10
#define ERROR_SEMICOLON			-11
#define ERROR_CONDITION_TYPE	-12
#define ERROR_VARITY_NONEXIST	-13
#define ERROR_CONST_ASSIGNED	-14
#define ERROR_STRUCT_ERROR		-15
#define ERROR_STRUCT_DUPLICATE	-16
#define ERROR_STRUCT_COUNT_MAX	-17
#define ERROR_STRUCT_NONEXIST	-18
#define ERROR_STRUCT_MEMBER		-19
#define ERROR_ILLEGAL_OPERAND	-20
#define ERROR_USED_INDEX		-21
#define ERROR_NEED_LEFT_VALUE	-22
#define ERROR_FUNC_ARG_LIST		-23
#define ERROR_OPERATOR			-24
#define ERROR_INVALID_OPERAND	-25
#define ERROR_TOKEN				-26
#define ERROR_INVALID_KEYWORD	-27
#define ERROR_OPERAND_LACKED	-28
#define ERROR_TYPE_CONVERT		-29
#define ERROR_TERNARY_UNMATCH	-30
#define ERROR_NO_SUB_STRUCT		-31
#define ERROR_NONSEQ_CTL		-32
#define ERROR_GOTO_POSITION		-33
#define ERROR_GOTO_COUNT_MAX	-34
#define ERROR_GOTO_LABEL		-35
#define ERROR_CTL_BREAK			-36
#define ERROR_FUNC_DEF_POS		-37
#define ERROR_TOKEN_TYPE		-38
#define ERROR_NO_VARITY_FOUND	-39
#define ERROR_NO_FUNCTION		-40
#define ERROR_SIZEOF			-41
#define ERROR_OPERAND_SURPLUS	-42
#define ERROR_STRUCT_NAME		-43
#define ERROR_VARITY_NAME		-44
#define ERROR_HEAD_TOKEN		-45
#define ERROR_DEVIDE_ZERO		-46
#define ERROR_EMPTY_FIFO		-47
#define ERROR_ILLEGAL_CODE		-48
#define ERROR_LINK				-49
#define ERROR_NOMAIN			-50
#define ERROR_FILE				-51
#define ERROR_ASSIGN			-52
#define ERROR_SR_STATUS			-53
#define ERROR_PREPROCESS		-54

#define ERROR_STACK_OVERFLOW	-70
#define ERROR_ARRAY_BOUND		-71
#define ERROR_HARD_FAULT		-72
#define ERROR_UNDEFINED_OPT		-73

#define ERROR_GDB_ARGC			-100

#define OK_NONSEQ_FINISH		1
#define OK_NONSEQ_INPUTING		2
#define OK_FUNC_INPUTING		3
#define OK_FUNC_FINISH			4
#define OK_FUNC_NOFUNC			5
#define OK_FUNC_CONTINUE_EXEC	6
#define OK_VARITY_DECLARE		7
#define OK_STRUCT_INPUTING		8
#define OK_STRUCT_NOSTRUCT		9
#define OK_STRUCT_FINISH		10
#define OK_NONSEQ_DEFINE		11
#define OK_FUNC_DEFINE			12
#define OK_LABEL_DEFINE			13
#define OK_FUNC_RETURN			14
#define OK_CTL_NOT_FOUND		15

#define OK_GDB_RUN				30
#define OK_GDB_STEPRUN_CODE		31
#define OK_GDB_STEPINTO			32
#define OK_GDB_STEPOVER			33
#endif
