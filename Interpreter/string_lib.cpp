#include "config.h"
#include "string_lib.h"
#include "varity.h"
#include "operator.h"
#include "error.h"
#include "cstdlib.h"
#include "interpreter.h"

extern char non_seq_key[7][7];

int get_escape_char(char *str, char &ch)
{
	if((str[1] >= '0' && str[1] <= '7')) {
		if((str[2] >= '0' && str[2] <= '7')) {
			if((str[3] >= '0' && str[3] <= '7')) {
				int tch = str[3] - '0' + (str[2] - '0') * 8 + (str[1] - '0') * 64;
				if(tch > 255) {
					error("%d is too large for char.\n", tch);
					return 0;
				}
				ch = tch;
				return 4;
			} else {
				ch = str[2] - '0' + (str[1] - '0') * 8;
				return 3;
			}
		} else {
			ch = str[1] - '0';
			return 2;
		}
	}
	if(str[1] == 'x') {
		if(kisdigit(str[2]) || str[2] >= 'a' && str[2] <= 'f' || str[2] >= 'A' && str[2] <= 'F') {
			if(str[3] >= '0' && str[3] <= '9' || str[3] >= 'a' && str[3] <= 'f' || str[3] >= 'A' && str[3] <= 'F') {
				if(kisdigit(str[2])) {
					ch = str[2] - '0';
				} else if(kisupper(str[2])) {
					ch = str[2] - 'A';
				} else if(kislower(str[2])) {
					ch = str[2] - 'a';
				}
				ch <<= 4;
				if(kisdigit(str[3])) {
					ch += str[3] - '0';
				} else if(kisupper(str[3])) {
					ch += str[3] - 'A';
				} else if(kislower(str[3])) {
					ch += str[3] - 'a';
				}
				return 4;
			} else {
				if(kisdigit(str[2])) {
					ch = str[2] - '0';
				} else if(kisupper(str[2])) {
					ch = str[2] - 'A';
				} else if(kislower(str[2])) {
					ch = str[2] - 'a';
				}
				return 3;
			}
		} else {
			error("Illegal hex number.\n");
			return 0;
		}
	}
	switch(str[1]) {
	case 'n':
		ch = '\n';
		return 2;
	case 'r':
		ch = '\r';
		return 2;
	case 'a':
		ch = '\a';
		return 2;
	case 'b':
		ch = '\b';
		return 2;
	case 'f':
		ch = '\f';
		return 2;
	case 't':
		ch = '\t';
		return 2;
	case 'v':
		ch = '\v';
		return 2;
	case '\\':
		ch = '\\';
		return 2;
	case '\'':
		ch = '\'';
		return 2;
	case '\"':
		ch = '\"';
		return 2;
	case '\?':
		ch = '\?';
		return 2;
	case 0:
		return 0;
	default:
		ch = str[1];
		return 2;
	}
	return 0;
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

int strmcmp(const char* str1, const char* str2, int len)
{
	for(int i=0; i<len; i++) {
		if(str1[i] != str2[i]) {
			return 1;
		}
	}
	return 0;
}

int make_align(PLATFORM_WORD value, int align_byte)
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
	return true;
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
