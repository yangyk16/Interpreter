#include "operator.h"
#include "interpreter.h"
#include "string_lib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

varity_info c_interpreter::plus_opt(char* str, uint* size_ptr)
{
	varity_info ret, *tmp_varity = 0;
	uint size = *size_ptr;
	varity_info* finded_varity;
	int opt_len = 0, opt_type;
	int symbol_pos_last = 0, symbol_pos_once, symbol_pos_next;
	while((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) > 0) {
		int continuous_plus_begin_pos = symbol_pos_last;
		int symbol_pos_cur = symbol_pos_last + symbol_pos_once + opt_len;
		size -= symbol_pos_once + opt_len;
		if(opt_type == OPT_PLUS || opt_type == OPT_MINUS) {
			char tmp_varity_name[3];
			this->varity_declare->declare_analysis_varity(0, 0, tmp_varity_name, &tmp_varity);
			char name_buf[32];
			memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
			name_buf[symbol_pos_once] = 0;
			int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
			if(tmp_varity_type == 0) {
				finded_varity = (varity_info*)this->varity_declare->global_varity_stack->find(name_buf);
				if(!finded_varity) {
					debug("varity \"%s\" doesn't exist\n", name_buf);
					return ret;
				}
				*tmp_varity = *finded_varity;
			} else if(tmp_varity_type == 1) {
				*tmp_varity = y_atof(name_buf);
			} else if(tmp_varity_type == 2) {
				*tmp_varity = y_atoi(name_buf);
			}
			symbol_pos_last += symbol_pos_once + opt_len;
			while((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) > 0) {
				size -= symbol_pos_once + opt_len;
				memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
				name_buf[symbol_pos_once] = 0;
				int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
				if(tmp_varity_type == 0) {
					finded_varity = (varity_info*)this->varity_declare->global_varity_stack->find(name_buf);
					if(!finded_varity) {
						debug("varity \"%s\" doesn't exist\n", name_buf);
						return ret;
					}
					*tmp_varity = *tmp_varity + *finded_varity;
				} else if(tmp_varity_type == 1) {
					varity_info sub_tmp_varity;
					sub_tmp_varity = y_atof(name_buf);
					*tmp_varity = sub_tmp_varity + *tmp_varity;
				} else if(tmp_varity_type == 2) {
					varity_info sub_tmp_varity;
					sub_tmp_varity = y_atoi(name_buf);
					*tmp_varity = sub_tmp_varity + *tmp_varity;
				}
				symbol_pos_last += symbol_pos_once + opt_len;
				if(opt_type != OPT_PLUS && opt_type != OPT_MINUS)
					break;
			}
			sub_replace(str, continuous_plus_begin_pos, symbol_pos_last - 1, tmp_varity_name);
			tmp_varity->echo();
		}
		//symbol_pos_last += symbol_pos_once;
		//debug("%d,%d\n",symbol_pos_last, opt_len);
		//symbol_pos_last += opt_len;
	}
	return ret;
}

varity_info c_interpreter::assign_opt(char* str, uint* len_ptr)
{
	varity_info ret;
	varity_info tmp_varity;
	varity_info* finded_varity;
	char tmpbuf[32];
	int len = *len_ptr;
	int i;
	int symbol_begin_pos;
	for(int num_flag=1, i=len-1, symbol_begin_pos=len; i>=-1; i--) {
		if(i == -1 || str[i] == '=' || str[i] == ',') {
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
			if(str[i] == ',')
				tmp_varity.reset();
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