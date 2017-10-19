#include "config.h"
#include "string_lib.h"
#include "varity.h"
#include "operator.h"
#include <stdlib.h>
#include <stdio.h>

char non_seq_key[][7] = {"", "if", "switch", "else", "for", "while", "do"};
const char opt_str[][4] = {"<<=",">>=","->","++","--","<<",">>",">=","<=","==","!=","&&","||","/=","*=","%=","+=","-=","&=","^=","|=","[","]","(",")",".","-","~","*","&","!","/","%","+",">","<","^","|","?",":","=",",",";"};

int optcmp(char* str)
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

int key_match(char* str, int size, int* type)
{
	int i, j;
	for(i=0; i<1; i++) {//i<size, TODO: ÓëoptcmpºÏ²¢
		for(j=0; j<sizeof(type_key)/sizeof(*type_key); j++) {
			if(str[i] == type_key[j][0]) {
				int k;
				for(k=1; k<type_len[j]; k++) {
					if(str[i + k] != type_key[j][k])
						break;
				}
				if(k == type_len[j] && (str[i + k] == ' ' || str[i + k] == '*' || str[i+k] == ')')) {
					*type = j;
					return i;
				}
			}
		}
	}
	*type = 0;
	return -1;
}

int nonseq_key_cmp(char* str)
{
	int i,j;
	for(i=0; i<sizeof(non_seq_key)/sizeof(non_seq_key[0]); i++) {
		for(j=0; non_seq_key[i][j]; j++) {
			if(str[j] != non_seq_key[i][j])
				break;
		}
		if(non_seq_key[i][j] == 0 && (str[j] == ' ' || str[j] == '(' || str[j] == 0))
			return i;
	}
	return 0;
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
	if(size <= 0) {
		*opt_len = 0;
		return -1;
	}
	if(!direction) {
		//direction=0: left->right; direction=1: right->left;
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
	int ret = OPERAND_INTEGER;
	for(int i=0; i<size && str[i]; i++)
		if(str[i] == '.')
			ret = OPERAND_FLOAT;
		else if((str[i] >'9' || str[i] < '0') && str[i] != '.') {
			ret = OPERAND_VARITY;
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

int str_find(char* str, char ch, int direction)
{
	int i = 0;
	if(direction == 0)
		while(str[i]) {
			if(str[i] == ch) {
				return i;
			} else
				i++;
		}
	else {
		int ret;
		while(str[i]) {
			if(str[i] == ch)
				ret = i;
			i++;
		}
		return ret;
	}
	return -1;
}

int str_find(char* str, int len, char ch, int direction)
{
	int i = 0;
	if(direction == 0)
		for(i=0; i<len; i++) {
			if(str[i] == ch)
				return i;
		}
	else {
		for(i=len-1; i>=0; i--) {
			if(str[i] == ch)
				return i;
		}
	}
	return -1;
}

int strequ(char* str1, char* str2, int len)
{
	for(int i=0; i<len; i++) {
		if(str1[i] != str2[i]) {
			return 1;
		}
	}
	return 0;
}

int char_count(char* str, char ch)
{
	int i = 0;
	int ret = 0;
	while(str[i] != 0) {
		if(str[i++] == ch) {
			ret++;
		}
	}
	return ret;
}

int make_align(int value, int align_byte)
{
	return value % align_byte == 0 ? value : value + align_byte - (value % align_byte);
}

bool is_valid_c_char(char ch)
{
	if(ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9' || ch == '_')
		return true;
	return false;
}

/*int varity_check(char* str, char tailed, char**& buf, int*& count)
{
	static char* varity_ptr[MAX_COUNT_VARITY_DEF];
	static char varity_buf[VARITY_DEF_BUF_LEN];
	static int varity_element_count[MAX_COUNT_VARITY_DEF];
	int varity_count = 0;
	int namelen = 0;
	int index_begin_pos, name_flag = 1;
	buf = varity_ptr;
	count = varity_element_count;
	buf[0] = varity_buf;
	count[0] = 1;
	for(int i=0, ptrlevel=0; str[i]!=tailed; i++) {
		if(str[i] == '*')
			ptrlevel++;
		else if(str[i] == ',') {
			ptrlevel = 0;
			buf[varity_count + 1] = &buf[varity_count][namelen + 1];
			buf[varity_count++][namelen] = 0;
			count[varity_count] = 1;
			namelen = 0;
			name_flag = 1;
		} else if(str[i] == '[') {
			index_begin_pos = i + 1;
			name_flag = 0;
		} else if(str[i] == ']') {
			if(str[i+1] != ',' && str[i+1] != ';') {
				error("It's not allowed that any letter exist after bracket\n");
				return -1;
			}
		} else {
			buf[varity_count][namelen++] = str[i];
		}
	}
	buf[varity_count++][namelen] = 0;
	return varity_count;
}*/