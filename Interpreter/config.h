#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#define MAX_STACK_INDEX			10
#define MAX_G_VARITY_NODE		128
#define MAX_L_VARITY_NODE		128
#define MAX_A_VARITY_NODE		32
#define MAX_SENTENCE_LENGTH		512
#define MAX_PRETREAT_BUFLEN		512
#define MAX_ANALYSIS_BUFLEN		768
#define MAX_SUB_ANA_BUFLEN		512
#define MAX_BRACKET_DEPTH		10
#define MAX_NON_SEQ_ROW			128
#define NON_SEQ_TMPBUF_LEN		2048
#define STRUCT_BUF_LEN			256
#define MAX_FUNCTION_LEN		4096
#define MAX_FUNCTION_LINE		128
#define MAX_FUNCTION_NODE		32
#define MAX_FUNCTION_ARGC		8
#define MAX_STRUCT_NODE			32
#define VARITY_ASSIGN_BUFLEN	256
#define MAX_COUNT_VARITY_DEF	8
#define PLATFORM_WORD_LEN		4

#define debug	printf
#define error	printf
#define vfree	free
#endif