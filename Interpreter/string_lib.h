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
int make_align(PLATFORM_WORD value, int align_byte);
bool is_valid_c_char(unsigned char ch);
bool is_letter(unsigned char ch);
bool is_non_zero(int type, void* addr);
int find_token_with_bracket_level(node_attribute_t*, int, node_attribute_t*, int);
int get_escape_char(char *str, char &ch);
int memcheck(void *src, char ch, unsigned int len);
int sub_replace(char* str, int index, int sublen, char* substr);
int cmd_enter(char *str, int no);
char* cmd_up(int no);
char* cmd_down(int no);
int cmd_dispose(int no);
unsigned int calc_sum32(int *p, int len);
#ifdef __cplusplus
extern "C" {
#endif
int read_codeline(void* tty, char* string, char ch, char *(*callback)(char*, int));
#ifdef __cplusplus
}
#endif
#endif
