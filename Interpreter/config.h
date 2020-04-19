#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#define PLATFORM_X86	1
#define PLATFORM_ARM	0
////////////////config//////////////////
#define INTERPRETER_DEBUG		0
#define SECTION_OPTIMIZE		0
#define DEBUG_EN				1
#define HW_PLATFORM				PLATFORM_X86//0:ARM 1:X86
#define TTY_TYPE				0
#define HEAPSIZE				(768 * 1024)
#define MAX_VARITY_TYPE_COUNT	16
#define DYNAMIC_ARRAY_EN		0
#define MAX_LABEL_COUNT			3
#define MAX_LABEL_NAME_LEN		16
#define MAX_VARITY_NAME_LEN		32
#define MAX_STACK_INDEX			8
#define MAX_FUNCTION_DEPTH		8//16
#define MAX_G_VARITY_NODE		128
#define MAX_L_VARITY_NODE		64
#define MAX_A_VARITY_NODE		4
#define DEFAULT_MACRO_NODE		16
#define DEFAULT_STRING_NODE		32
#define DEFAULT_NAME_NODE		32
#define DEFAULT_NAME_LENGTH		508//512 - sizeof(str_list)
#define DEFAULT_STRING_LENGTH	508
#define MAX_SENTENCE_LENGTH		512
#define MAX_PRETREAT_BUFLEN		512
#define MAX_TOKEN_BUFLEN		2048
#define MAX_BRACKET_DEPTH		16
#define MAX_IFDEF_DEPTH			4
#define MAX_MACRO_ARG_COUNT		3
#define MAX_NON_SEQ_ROW			128
#define NON_SEQ_TMPBUF_LEN		2048
#define STRUCT_BUF_LEN			256
#define MAX_LOGIC_DEPTH			16
#define MAX_FUNCTION_LEN		4096
#define MAX_FUNCTION_LINE		128
#define MAX_FUNCTION_NODE		32
#define MAX_FUNCTION_ARGC		8
#define MAX_SIZEOF_DEPTH		2
#define MAX_STRUCT_NODE			32
#define MAX_ANALYSIS_NODE		128
#define VARITY_ASSIGN_BUFLEN	256
#define PLATFORM_WORD_LEN		4
#define MAX_MID_CODE_COUNT		1024
#define STACK_SIZE				0x1000//128k
#define TMP_VARITY_STACK_SIZE	(80)//sizeof(varity_info)*MAX_A_VARITY_NODE
#define MAX_INDENTATION			8

#define STACK_OVERFLOW_CHECK		1
#define ARRAY_BOUND_CHECK			1
#define HARD_FAULT_CHECK			1
#define DIVIDE_ZERO_CHECK			1

#if SECTION_OPTIMIZE
#define ITCM_TEXT __attribute__((section(".itcmcode")))
#define DTCM_BSS  __attribute__((section(".dtcmdata")))
#else
#define ITCM_TEXT
#define DTCM_BSS
#endif

#if INTERPRETER_DEBUG
#define dmalloc(size, info) vmalloc(size, info)
//void *vmalloc(unsigned int, const char*);
#else
#define dmalloc(size, info) vmalloc(size)
//void *vmalloc(unsigned int);
#endif

#define memdebug	
#define refdebug	
#define filedebug	debug
#endif
