#include "config.h"
#include "string_lib.h"
#include "varity.h"
#include "operator.h"
#include "error.h"
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

int c_interpreter::get_token(char *str, node_attribute_t *info)
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
		if(!kstrcmp(symbol_ptr, "sizeof")) {
			info->node_type = TOKEN_OPERATOR;
			info->data = OPT_SIZEOF;
			info->value_type = 2;
			return i;
		}
		info->value.ptr_value = symbol_ptr;
		info->node_type = TOKEN_NAME;
		return i;
	} else if(kisdigit(str[i]) || (str[i] == '.' && kisdigit(str[i + 1]))) {
		if(str[i] == '0' && (str[i + 1] == 'x' || str[i + 1] == 'X')) {
			if(!(str[i + 2] >= '0' && str[i + 2] <= '9' || str[i + 2] >= 'a' && str[i + 2] <= 'f' || str[i + 2] >= 'A' && str[i + 2] <= 'F')) {
				error("Illegal hex number.\n");
				return ERROR_TOKEN;
			}
			i += 2;
			while(1) {
				if(!(str[i] >= '0' && str[i] <= '9' || str[i] >= 'a' && str[i] <= 'f' || str[i] >= 'A' && str[i] <= 'F')) {
					goto int_value_handle;
				}
				i++;
			}
		}
		i++;
		while(kisdigit(str[i]))i++;
		if(str[i] == '.') {
			i++;
			float_flag = 1;
			while(kisdigit(str[i]))i++;
		}
		if(str[i] == 'e' || str[i] == 'E') {
			i++;
			float_flag = 1;
			if(str[i] == '-' || str[i] == '+')
				i++;
			while(kisdigit(str[i]))i++;
		}
		if(float_flag) {
			info->value_type = DOUBLE;
			info->value.double_value = y_atof(str, i);
			info->node_type = TOKEN_CONST_VALUE;
			return i;
		}
int_value_handle:
		info->value.int_value = y_atoi(str, i);
		if(str[i] == 'u') {
			info->value_type = U_INT;
			i++;
		} else {
			info->value_type = INT;
		}
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
	} else if(str[i] == '{' || str[i] == '}') {
		info->node_type = TOKEN_OTHER;
		return 1;
	} else {
		for(int j=0; j<sizeof(opt_str)/sizeof(opt_str[0]); j++) {
			if(!strmcmp(str + i, opt_str[j], opt_str_len[j])) {
				info->data = j;
				info->node_type = TOKEN_OPERATOR;
				info->value_type = opt_prio[j];
				if(info->data == OPT_L_SMALL_BRACKET) {
					struct_info *struct_info_ptr;
					node_attribute_t node;
					int varity_type, varity_len, total_len;
					int len_in_bracket = find_ch_with_bracket_level(str + i, ')', 1);
					int v_len = len_in_bracket;
					PLATFORM_WORD *complex_info_ptr;
					char name[32];
					varity_type = basic_type_check(str + i + 1, v_len, struct_info_ptr);
					total_len = i + 1;
					if(varity_type > 0) {
						int i_bak = i;
						i += v_len + 1;
						total_len += v_len;
						varity_len = (len_in_bracket -= v_len) - 1;
						varity_type = get_varity_type(str + i, varity_len, name, varity_type, struct_info_ptr, complex_info_ptr);
						if(varity_type > 0) {
							total_len += varity_len;
							i += varity_len;
							len_in_bracket -= varity_len;
							if(get_token(str + i, &node) == len_in_bracket) {
								if(name[0] != '\0') {
									//error("Convert operator contains varity name.\n");
									info->node_type = TOKEN_ARG_LIST;
									return total_len + 1;
								}
								info->data = OPT_TYPE_CONVERT;
								info->value_type = 2;
								info->value.ptr_value = (char*)complex_info_ptr;
								info->count = varity_type; //complex_arg_count
								return total_len + 1;
							}
						}
						i = i_bak;
					}
				}
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

int y_atoi(char* str)
{
	return katoi(str);
}

int y_atoi(char* str, int size)
{
	int ret;
	char ch = str[size];
	str[size] = 0;
	ret = katoi(str);
	str[size] = ch;
	return ret;
}

double y_atof(char* str)
{
	return katof(str);
}

double y_atof(char* str, int size)
{
	double ret;
	char ch = str[size];
	str[size] = 0;
	ret = katof(str);
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
