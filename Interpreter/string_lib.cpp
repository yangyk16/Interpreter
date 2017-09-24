#include "config.h"
#include "string_lib.h"
#include "varity.h"
#include "operator.h"
#include <stdlib.h>
#include <stdio.h>

char non_seq_key[][7] = {"if", "switch", "else", "for", "while", "do"};
const char opt_str[][4] = {"<<=",">>=","->","++","--","<<",">>",">=","<=","==","!=","&&","||","/=","*=","%=","+=","-=","&=","^=","|=","[","]","(",")",".","-","~","*","&","!","/","%","+",">","<","^","|","?",":","=",",",";"};

int keycmp(char* str)
{
	int i,j;
	for(i=0; i<sizeof(type_key)/sizeof(type_key[0]); i++) {
		for(j=0; type_key[i][j]; j++) {
			if(str[j] != type_key[i][j])
				break;
		}
		if(type_key[i][j] == 0 && (str[j] == ' ' || str[j] == '*'))
			return i;
	}
	return -1;
}

char* trim(char* str)
{
	return 0;
}

int remove_char(char* str, char ch)
{
	int rptr = 0, wptr = 0;
	while(str[rptr]) {
		if(str[rptr] != ch) {
			str[wptr++] = str[rptr];
		}
		rptr++;
	}
	str[wptr] = 0;
	return wptr;
}

int remove_substring(char* str, int index1, int index2)
{
	if(index2 < index1)
		return -1;
	int wptr = index1, rptr = index2 + 1;
	for(; str[rptr]; rptr++) {
		str[wptr++] = str[rptr];
	}
	str[wptr] = 0;
	return index2 - index1 + 1;
}

int search_opt(char* str, int size, int direction, int* opt_len, int* opt_type)
{
	int i, j;
	if(!direction) {
		//direction=1: left->right; direction=0: right->left;
		//merge 2 cycles to 1 can save space, but not merge is easily to read.
		for(i=0; i<size; i++) {
			for(j=0; j<sizeof(opt_str)/sizeof(*opt_str);j++) {
				if(str[i] == opt_str[j][0]) {
					if(i < size - 1 && str[i+1] == opt_str[j][1]) {
						if(i < size - 2 && str[i+2] == opt_str[j][2]) {
							*opt_len = 3;
							*opt_type = j;
							return i;
						}
						if(!opt_str[j][2]) {
							*opt_len = 2;
							*opt_type = j;
							return i;
						}
					}
					if(!opt_str[j][1]) {
						*opt_len = 1;
						*opt_type = j;
						return i;
					}
				}
			}
		}
		*opt_len = 0;
		*opt_type = OPT_EDGE;
		return size;
	} else {
		for(i=size-1; i>=0; i--)
			for(j=0; j<sizeof(opt_str)/sizeof(*opt_str);j++) {
				if(str[i] == opt_str[j][0]) {
					if(i < size - 1 && str[i+1] == opt_str[j][1]) {
						if(i < size - 2 && str[i+2] == opt_str[j][2]) {
							*opt_len = 3;
							*opt_type = j;
							return i;
						}
						if(!opt_str[j][2]) {
							*opt_len = 2;
							*opt_type = j;
							return i;
						}
					}
					if(!opt_str[j][1]) {
						*opt_len = 1;
						*opt_type = j;
						return i;
					}
				}
			}
		if(i == -1) {
			*opt_len = 0;
			*opt_type = OPT_EDGE;
			return 0;
		}
	}
	return -1;
}

int check_symbol(char* str, int size)
{
	int ret = 2;
	for(int i=0; i<size; i++)
		if(str[i] == '.')
			ret = 1;
		else if((str[i] >'9' || str[i] < '0') && str[i] != '.') {
			ret = 0;
			break;
		}
	return ret;
}

int sub_replace(char* str, int indexl, int indexr, char* sub_str)
{
	int sub_str_len = strlen(sub_str);
	if(sub_str_len <= indexr - indexl + 1) {
		memcpy(str + indexl, sub_str, sub_str_len);
		strcpy(str + indexl + sub_str_len, str + indexr + 1);
	} else {
		debug("complete code,string_lib.cpp, %d" ,__LINE__);
	}
	return sub_str_len - (indexr - indexl + 1);
}

int y_atoi(char* str)
{
	return atoi(str);
}

int y_atoi(char* str, int size)
{
	int ret;
	char ch = str[size];
	str[size] = 0;
	ret = atoi(str);
	str[size] = ch;
	return ret;
}

double y_atof(char* str)
{
	return atof(str);
}

double y_atof(char* str, int size)
{
	double ret;
	char ch = str[size];
	str[size] = 0;
	ret = atof(str);
	str[size] = ch;
	return ret;
}