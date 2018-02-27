#pragma once
#ifndef STRING_LIB_H
#define STRING_LIB_H
#include "varity.h"

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

inline int IsSpace(char ch) {return (ch == ' ' || ch == '\t');}

typedef struct node_attribute_s node_attribute_t;

int optcmp(char* str);
int nonseq_key_cmp(char* str);
int remove_char(char* str, char ch);
int y_atoi(char* str, int size);
int y_atoi(char* str);
double y_atof(char* str, int size);
double y_atof(char* str);
int str_find(char* str, char ch, int direction = 0);
int str_find(char* str, int len, char ch, int direction = 0);
int key_match(char* str, int size, int* type);
int strmcmp(const char* str1, const char* str2, int len);
int char_count(char* str, char ch);
int make_align(int value, int align_byte);
bool is_valid_c_char(unsigned char ch);
bool is_letter(unsigned char ch);
bool is_non_zero(int type, void* addr);
int find_ch_with_bracket_level(char* str, char ch, int level);
int get_token(char *str, node_attribute_t *info);
#endif
