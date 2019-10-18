#include "config.h"
#include "string_lib.h"
#include "varity.h"
#include "operator.h"
#include "error.h"
#include "cstdlib.h"
#include "interpreter.h"
#include "kmalloc.h"

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

int make_align(PLATFORM_WORD value, int align_byte)
{
	return value % align_byte == 0 ? value : value + align_byte - (value % align_byte);
}

int memcheck(void *src, char ch, unsigned int len)
{
	char *s = (char*)src;
	for(int i=0; i<len; i++) {
		if(s[i] != ch)
			return 1;
	}
	return 0;
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

int find_token_with_bracket_level(node_attribute_t* node_list, int n, node_attribute_t *node, int level)
{
	for(int i=0, cur_level=0; i<n; i++) {
		if(!kmemcmp(&node_list[i], node, sizeof(node_attribute_t)) && level == cur_level) {
			return i;
		} else if((node_list[i].data == OPT_L_SMALL_BRACKET || node_list[i].data == OPT_L_MID_BRACKET) && node_list[i].node_type == TOKEN_OPERATOR) {
			cur_level++;
		} else if((node_list[i].data == OPT_R_SMALL_BRACKET || node_list[i].data == OPT_R_MID_BRACKET) && node_list[i].node_type == TOKEN_OPERATOR) {
			cur_level--;
		}
	}
	return -1;
}

int sub_replace(char* str, int index, int sublen, char* substr)
{
	int sub_str_len = kstrlen(substr);
	kmemmove(str + index + sub_str_len ,str + index + sublen, kstrlen(str + index + sublen) + 1);
	kmemcpy(str + index, substr, sub_str_len);
	return sub_str_len - sublen;
}

extern "C" int read_codeline(void* tty, char* string, char ch, char *(*callback)(char*, int))
{
	return ((terminal*)tty)->readline(string, ch, callback);
}

#define CMD_LOG_COUNT   5
unsigned char cmd_count[2] = {0};
char cmd_pos[2] = {-1, -1};
char *cmd_str[2][CMD_LOG_COUNT] = {0};
int cmd_init(int no)
{
	int i;
	for(i=0; i<CMD_LOG_COUNT; i++)
		cmd_str[no][i] = 0;
	return 0;
}

int cmd_enter(char *str, int no)
{
	int i;
	int hitret = 0;
	cmd_pos[no] = -1;
	for(i=0; i<cmd_count[no]; i++) {
		if(!kstrcmp(str, cmd_str[no][i])) {//TODO: use stable sort instead of exchange
			char *tmp = cmd_str[no][0];
			cmd_str[no][0] = cmd_str[no][i];
			cmd_str[no][i] = tmp;
			return 0;
		}
	}
	if(cmd_count[no] < 5) 
		cmd_count[no]++;
	else	
		kfree(cmd_str[no][4]);
	for(i=cmd_count[no]-1; i>0; i--)
		cmd_str[no][i] = cmd_str[no][i - 1];
	cmd_str[no][0] = (char*)kmalloc(kstrlen(str) + 1);
	kstrcpy(cmd_str[no][0], str);
	return 0;
}

char* cmd_up(int no)
{
	if(cmd_count[no] == 0) return 0; 
	cmd_pos[no] = (cmd_pos[no] + 1 + cmd_count[no]) % cmd_count[no];
	return cmd_str[no][cmd_pos[no]];
}

char* cmd_down(int no)
{
	if(cmd_count[no] == 0) return 0; 
	cmd_pos[no] = (cmd_pos[no] - 1 + cmd_count[no]) % cmd_count[no];
	return cmd_str[no][cmd_pos[no]];
}

int cmd_dispose(int no)
{
	int i;
	for(i=0; i<CMD_LOG_COUNT; i++) 
		if(cmd_str[no][i]) kfree(cmd_str[no][i]);
	return 0;
}
