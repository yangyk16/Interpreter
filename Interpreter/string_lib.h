#pragma once
#ifndef STRING_LIB_H
#define STRING_LIB_H
#include <string.h>

inline int IsSpace(char ch) {return (ch == ' ' || ch == '\t');}
//need a bracket stack save outer bracket
inline int get_bracket_depth(char* str)
{
	char bracket_type_stack[MAX_BRACKET_DEPTH];
	int max_depth = 0;
	int current_depth = 0;//parentheses
	int current_depth_m = 0;//bracket
	int len = strlen(str);

	for(int i=0; i<len; i++) {
		if(str[i] == '(') {
			bracket_type_stack[current_depth + current_depth_m] = 0;
			current_depth++;
			if(current_depth + current_depth_m > max_depth)
				max_depth = current_depth + current_depth_m;
		} else if(str[i] == ')') {
			current_depth--;
			if(bracket_type_stack[current_depth + current_depth_m] != 0)
				return -4;
			if(current_depth < 0)
				return -1;
		}
		if(str[i] == '[') {
			bracket_type_stack[current_depth + current_depth_m] = 1;
			current_depth_m++;
			if(current_depth + current_depth_m > max_depth)
				max_depth = current_depth + current_depth_m;
		} else if(str[i] == ']') {
			current_depth_m--;
			if(bracket_type_stack[current_depth + current_depth_m] != 1)
				return -5;
			if(current_depth_m < 0)
				return -2;
		}
	}
	if(current_depth != 0 || current_depth_m != 0)
		return -3;
	else
		return max_depth;
}

int keycmp(char* str);
int remove_char(char* str, char ch);
int remove_substring(char* str, int index1, int index2);
int search_opt(char* str, int size, int direction, int* opt_len, int* opt_type);
int check_symbol(char* str, int size);
int y_atoi(char* str, int size);
int y_atoi(char* str);
double y_atof(char* str, int size);
double y_atof(char* str);
#endif
