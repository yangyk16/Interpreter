#include "operator.h"
#include "interpreter.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

varity_info c_interpreter::plus_opt(char* str, uint size)
{
	varity_info ret;
	return ret;
}

varity_info c_interpreter::assign_opt(char* str, uint len)
{
	varity_info ret;
	varity_info tmp_varity;
	varity_info* finded_varity;
	char tmpbuf[32];
	int i;
	int symbol_begin_pos;
	for(int num_flag=1, i=len-1, symbol_begin_pos=len; i>=-1; i--) {
		if(i == -1 || str[i] == '=') {
			memcpy(tmpbuf, str + i + 1, symbol_begin_pos - i - 1);
			tmpbuf[symbol_begin_pos - i - 1] = 0;
			if(num_flag == 1) {
				remove_substring(str, i + 1, symbol_begin_pos - 1);
				tmp_varity = atoi(tmpbuf);
			} else if(num_flag == 2) {
				remove_substring(str, i + 1, symbol_begin_pos - 1);
				tmp_varity = atof(tmpbuf);
			} else if(num_flag == 0) {
				finded_varity = (varity_info*)this->varity_declare->global_varity_stack->find(tmpbuf);
				if(!finded_varity) {
					debug("varity \"%s\" doesn't exist\n", tmpbuf);
					return ret;
				}
				if(str[symbol_begin_pos] == '=') {
					remove_substring(str, symbol_begin_pos, symbol_begin_pos);
					*finded_varity = tmp_varity;
					finded_varity->echo();
				}
				tmp_varity = *finded_varity;
			}
			symbol_begin_pos = i;
			num_flag = 1;
		} else if(str[i] == '.')
			if(num_flag == 1)
				num_flag = 2;
			else
				num_flag = 3;
		else if(str[i] == '-')
			;
		else if(str[i] < '0' || str[i] > '9')
			num_flag = false;
		
	}
	return ret;
}