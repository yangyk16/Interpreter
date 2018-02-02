#include "config.h"
#include "string_lib.h"
#include "varity.h"
#include "operator.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include "cstdlib.h"
#include "interpreter.h"

extern char non_seq_key[7][7];
extern const char non_seq_key_len[7];
extern char opt_str[43][4];
extern char opt_str_len[43];
extern char opt_prio[43];
extern char opt_number[54];

round_queue token_fifo;
int get_escape_char(char *str, char &ch) {
	switch(str[1]) {
	case 'n':
		ch = '\n';
		return 2;
	case 0:
		return -1;
	}
}

int get_token(char *str, node_attribute_t *info)
{
	int i = 0, real_token_pos, float_flag = 0;
	char *symbol_ptr = token_fifo.get_wptr() + (char*)token_fifo.get_base_addr();
	while(str[i] == ' ' || str[i] == '\t')i++;
	real_token_pos = i;
	if(is_letter(str[i])) {
		i++;
		while(is_valid_c_char(str[i]))i++;
		token_fifo.write(str + real_token_pos, i - real_token_pos);
		token_fifo.write("\0",1);
		for(int j=0; j<sizeof(type_key)/sizeof(type_key[0]); j++) {
			if(!kstrcmp(symbol_ptr, type_key[j])) {
				info->value.int_value = j;
				info->node_type = TOKEN_KEYWORD_TYPE;
				return i;
			}
		}
		for(int j=0; j<sizeof(non_seq_key)/sizeof(non_seq_key[0]); j++) {
			if(!kstrcmp(symbol_ptr, non_seq_key[j])) {
				info->value.int_value = j;
				info->node_type = TOKEN_KEYWORD_NONSEQ;
				return i;
			}
		}
		info->value.ptr_value = symbol_ptr;
		info->node_type = TOKEN_NAME;
		return i;
	} else if(is_number(str[i]) || (str[i] == '.' && is_number(str[i + 1]))) {
		i++;
		while(is_number(str[i]))i++;
		if(str[i] == '.') {
			i++;
			float_flag = 1;
			while(is_number(str[i]))i++;
		}
		if(str[i] == 'e' || str[i] == 'E') {
			i++;
			float_flag = 1;
			while(is_number(str[i]))i++;
		}
		if(float_flag) {
			info->value_type = DOUBLE;
			info->value.double_value = y_atof(str, i);
			info->node_type = TOKEN_CONST_VALUE;
			return i;
		}
		info->value_type = INT;
		info->value.int_value = y_atoi(str, i);
		info->node_type = TOKEN_CONST_VALUE;
		return i;
	} else if(str[i] == '"') {
		int count = 0;
		int pos = ++i;
		while(str[i]) {
			if(str[i] == '\\') {//TODO:处理\ddd\xhh
				i++;
			} else if(str[i] == '"') {
				char *p = (char*)vmalloc(count + 1);
				for(int j=0; j<count; j++) {
					if(str[pos] == '\\') {
						char ch = str[pos + 1];
						switch(ch) {
						case 'n':
							p[j] = '\n';
						}
						pos += 2;
					} else {
						p[j] = str[pos++]; 
					}
				}
				p[count] = 0;
				info->node_type = TOKEN_STRING;
				info->value.ptr_value = p;
				return i + 1;
			}
			i++;
			count++;
		}
	} else if(str[i] == '\'') {
		char character;
		int ret = 1;
		i++;
		if(str[i] == '\\') {
			ret = get_escape_char(str, character);
		} else {
			character = str[i];
		}
		i += ret;
		if(str[i] == '\'') {
			info->value_type = CHAR;
			info->value.int_value = character;
			info->node_type = TOKEN_CONST_VALUE;
			return i + 1;
		} else {
			info->node_type = TOKEN_ERROR;
			error("\' operator error.\n");
			return ERROR_TOKEN;
		}
	} else {
		for(int j=0; j<sizeof(opt_str)/sizeof(opt_str[0]); j++) {
			if(!strmcmp(str + i, opt_str[j], opt_str_len[j])) {
				info->value.int_value = j;
				info->node_type = TOKEN_OPERATOR;
				info->value_type = opt_prio[j];
				return i + opt_str_len[j];
			}
		}
		info->node_type = TOKEN_ERROR;
		return ERROR_TOKEN;
	}
	return 0;
}

int optcmp(char* str)
{
	int i,j;
	for(i=0; i<sizeof(type_key)/sizeof(type_key[0]); i++) {
		for(j=0; type_key[i][j]; j++) {
			if(str[j] != type_key[i][j])
				break;
		}
		if(type_key[i][j] == 0 && (str[j] == ' ' || str[j] == '*' || str[j] == 0))
			return i;
	}
	return -1;
}

int is_type_convert(char* str, varity_info* covert_type_ptr)
{
	int ret, varity_type;
	varity_type = optcmp(str);
	if(varity_type >= 0) {
		int ptr_level = 0;
		int i = kstrlen(type_key[varity_type]);
		while(str[i]) {
			if(str[i] != '*' && str[i] != 0)
				return 0;
			else
				ptr_level++;
		}
		if(covert_type_ptr) {
			covert_type_ptr->set_type(varity_type + ptr_level * BASIC_VARITY_TYPE_COUNT);
		}
		return 1;
	} else 
		return 0;
}

int key_match(char* str, int size, int* type)
{
	int i, j;
	for(i=0; i<1; i++) {//i<size, TODO: 与optcmp合并
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
			for(j=0; j<sizeof(opt_str)/sizeof(opt_str[0]);j++) {
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
						if(*opt_type == OPT_PLUS || *opt_type == OPT_MINUS) {
							if(i != 0 && (str[i-1] == 'e' || str[i-1] == 'E')) {
								int k;
								for(k=0; k<i-1; k++) {
									if(str[k] != '.' && (str[k] < '0' || str[k] > '9')) {
										break;
									}
								}
								if(k == i-1) {
									for(k=i+1; is_valid_c_char(str[k]); k++) {
										if(str[k] != '.' && (str[k] < '0' || str[k] > '9')) {
											error("float const is invalid\n");
											return ERROR_ILLEGAL_OPERAND;
										}
									}
									break;
								}
							}
						} else if(*opt_type == OPT_MEMBER) {
							int k;
							for(k=0; k<i; k++) {
								if(str[k] != '.' && (str[k] < '0' || str[k] > '9')) {
									break;
								}
							}
							if(k == i) {
								for(k=i+1; is_valid_c_char(str[k]) || str[k]=='.'; k++) {
									if((str[k] < '0' || str[k] > '9') && (str[k] != 'e' && str[k] != 'E')) {
										error("float const is invalid\n");
										return ERROR_ILLEGAL_OPERAND;
									}
								}
								if(str[k-1] == 'e' || str[k-1] == 'E') {
									error("float const is invalid\n");
									return ERROR_ILLEGAL_OPERAND;
								}
								break;
							}
						}
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
			for(j=0; j<sizeof(opt_str)/sizeof(opt_str[0]);j++) {
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
		if(str[i] == '.') {
			if(ret == OPERAND_INTEGER)
				ret = OPERAND_FLOAT;
			else if(ret == OPERAND_FLOAT) {
				error("float const error\n");
				return ERROR_ILLEGAL_OPERAND;
			}
		} else if((str[i] > '9' || str[i] < '0') && str[i] != '.') {
			if(str[i] == 'e' || str[i] == 'E') {
				if(i == size - 1) {
					error("float const error\n");
					return ERROR_ILLEGAL_OPERAND;
				}
				for(int j=i+1; j<size && str[j]; j++) {
					if((str[j] > '9' || str[j] < '0') && str[j] != '+' && str[j] != '-') {
						error("float const error\n");
						return ERROR_ILLEGAL_OPERAND;
					}
				}
				ret = OPERAND_FLOAT;
				break;
			} else {
				ret = OPERAND_VARITY;
				break;
			}
		}
	return ret;
}

int sub_replace(char* str, int indexl, int indexr, char* sub_str)
{
	int sub_str_len = kstrlen(sub_str);
	if(sub_str_len <= indexr - indexl + 1) {
		kmemcpy(str + indexl, sub_str, sub_str_len);
		kstrcpy(str + indexl + sub_str_len, str + indexr + 1);
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

int strmcmp(const char* str1, const char* str2, int len)
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

bool is_valid_c_char(unsigned char ch)
{
	if(ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9' || ch == '_' || ch >= 128)//TODO: 128 use macro, also as varity.cpp
		return true;
	return false;
}

bool is_letter(unsigned char ch)
{
	if(ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch == '_')
		return true;
	return false;
}

bool is_number(unsigned char ch)
{
	if(ch >= '0' && ch <= '9')
		return true;
	return false;
}

bool is_non_zero(int type, void* addr)
{
	switch(type) {
	case DOUBLE:
		return DOUBLE_VALUE(addr) != 0;
	case FLOAT:
		return FLOAT_VALUE(addr) != 0;
	case INT:
	case U_INT:
		return INT_VALUE(addr) != 0;
	case SHORT:
	case U_SHORT:
		return SHORT_VALUE(addr) != 0;
	case CHAR:
	case U_CHAR:
		return CHAR_VALUE(addr) != 0;
	case LONG_LONG:
	case U_LONG_LONG:
		return LONG_LONG_VALUE(addr) != 0;
	}
}

int find_ch_with_bracket_level(char* str, char ch, int level)
{
	for(int i=0, cur_level=0; str[i]; i++) {
		if(str[i] == ch && level == cur_level) {
			return i;
		} else if(str[i] == '(' || str[i] == '[') {
			cur_level++;
		} else if(str[i] == ')' || str[i] == ']') {
			cur_level--;
		}
	}
	return -1;
}