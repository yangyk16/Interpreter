#pragma once
#ifndef STRING_LIB_H
#define STRING_LIB_H
#include "varity.h"

inline int IsSpace(char ch) {return (ch == ' ' || ch == '\t');}

typedef struct node_attribute_s node_attribute_t;

int y_atoi(char* str, int size);
int y_atoi(char* str);
double y_atof(char* str, int size);
double y_atof(char* str);
int strmcmp(const char* str1, const char* str2, int len);
int make_align(PLATFORM_WORD value, int align_byte);
bool is_valid_c_char(unsigned char ch);
bool is_letter(unsigned char ch);
bool is_non_zero(int type, void* addr);
int find_ch_with_bracket_level(char* str, char ch, int level);
int get_escape_char(char *str, char &ch);
#endif
