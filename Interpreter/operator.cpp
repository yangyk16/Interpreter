#include "operator.h"
#include "interpreter.h"
#include "string_lib.h"
#include "varity.h"
#include "type.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int c_interpreter::plus_opt(char* str, uint* size_ptr)
{
	varity_info ret, *tmp_varity = 0;
	uint size = *size_ptr;
	varity_info* finded_varity;
	int opt_len = 0, opt_type;
	int symbol_pos_last = 0, symbol_pos_once;
	while((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) > 0) {
		int continuous_plus_begin_pos = symbol_pos_last;
		int symbol_pos_cur = symbol_pos_last + symbol_pos_once + opt_len;
		size -= symbol_pos_once + opt_len;
		if(opt_type == OPT_PLUS || opt_type == OPT_MINUS) {
			int delta_str_len;
			char tmp_varity_name[3];
			this->varity_declare->declare_analysis_varity(0, 0, tmp_varity_name, &tmp_varity);
			char name_buf[32];
			memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
			name_buf[symbol_pos_once] = 0;
			int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
			if(tmp_varity_type == 0) {
				finded_varity = (varity_info*)this->varity_declare->analysis_varity_stack->find(name_buf);
				if(!finded_varity)
					finded_varity = (varity_info*)this->varity_declare->global_varity_stack->find(name_buf);
				if(!finded_varity) {
					debug("varity \"%s\" doesn't exist\n", name_buf);
					return 1;
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
					finded_varity = (varity_info*)this->varity_declare->analysis_varity_stack->find(name_buf);
					if(!finded_varity)
						finded_varity = (varity_info*)this->varity_declare->global_varity_stack->find(name_buf);
					if(!finded_varity) {
						debug("varity \"%s\" doesn't exist\n", name_buf);
						return 1;
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
			delta_str_len = sub_replace(str, continuous_plus_begin_pos, symbol_pos_last - 1, tmp_varity_name);
			*size_ptr += delta_str_len;
			symbol_pos_last += delta_str_len;
			tmp_varity->echo();
		} else {
			symbol_pos_last += symbol_pos_once + opt_len;
		}
	}
	return 0;
}

int c_interpreter::assign_opt(char* str, uint* len_ptr)
{
	varity_info ret, *tmp_varity = 0;
	uint size = *len_ptr;
	varity_info* finded_varity;
	int opt_len = 0, opt_type;
	int symbol_pos_once;
	while(size > 0) {
		symbol_pos_once = search_opt(str, size, 1, &opt_len, &opt_type);
		int continuous_assign_end_pos = size - 1;
		int symbol_pos_cur = symbol_pos_once - 1;
		if(opt_type == OPT_ASSIGN || opt_type == OPT_ADD_ASSIGN) {
			int delta_str_len;
			char tmp_varity_name[3];
			this->varity_declare->declare_analysis_varity(0, 0, tmp_varity_name, &tmp_varity);
			char name_buf[32];
			memcpy(name_buf, str + symbol_pos_once + opt_len, size - symbol_pos_once - opt_len);
			name_buf[size - symbol_pos_once - opt_len] = 0;
			int tmp_varity_type = check_symbol(name_buf, size - symbol_pos_once - opt_len);
			if(tmp_varity_type == 0) {
				finded_varity = (varity_info*)this->varity_declare->analysis_varity_stack->find(name_buf);
				if(!finded_varity)
					finded_varity = (varity_info*)this->varity_declare->global_varity_stack->find(name_buf);
				if(!finded_varity) {
					debug("varity \"%s\" doesn't exist\n", name_buf);
					return 1;
				}
				*tmp_varity = *finded_varity;
			} else if(tmp_varity_type == 1) {
				*tmp_varity = y_atof(name_buf);
			} else if(tmp_varity_type == 2) {
				*tmp_varity = y_atoi(name_buf);
			}
			size = symbol_pos_cur + 1;
			while(size > 0) {
				symbol_pos_once = search_opt(str, size, 1, &opt_len, &opt_type);
				symbol_pos_cur = symbol_pos_once - 1;
				memcpy(name_buf, str + symbol_pos_once + opt_len, size - symbol_pos_once - opt_len);
				name_buf[size - symbol_pos_once - opt_len] = 0;
				int tmp_varity_type = check_symbol(name_buf, size - symbol_pos_once - opt_len);
				if(tmp_varity_type == 0) {
					finded_varity = (varity_info*)this->varity_declare->analysis_varity_stack->find(name_buf);
					if(!finded_varity)
						finded_varity = (varity_info*)this->varity_declare->global_varity_stack->find(name_buf);
					if(!finded_varity) {
						debug("varity \"%s\" doesn't exist\n", name_buf);
						return 1;
					}
					*finded_varity = *tmp_varity;
					finded_varity->echo();
				} else if(tmp_varity_type == 1) {
					//*tmp_varity = *tmp_varity;
				} else if(tmp_varity_type == 2) {
					//*tmp_varity = *tmp_varity;
				}
				size = symbol_pos_cur + 1;
				if(opt_type != OPT_ASSIGN && opt_type != OPT_ADD_ASSIGN)
					break;
			}
			delta_str_len = sub_replace(str, symbol_pos_once + opt_len, continuous_assign_end_pos, tmp_varity_name);
			*len_ptr += delta_str_len;
			size = symbol_pos_cur + 1;
		} else {
			size = symbol_pos_cur + 1;
		}
	}
	return 0;
}

int c_interpreter::relational_opt(char* str, uint* size_ptr)
{
	varity_info ret, *tmp_varity = 0;
	uint size = *size_ptr;
	varity_info* finded_varity;
	int opt_len = 0, opt_type, last_opt_type;
	int symbol_pos_last = 0, symbol_pos_once;
	while((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) > 0) {
		int continuous_plus_begin_pos = symbol_pos_last;
		int symbol_pos_cur = symbol_pos_last + symbol_pos_once + opt_len;
		size -= symbol_pos_once + opt_len;
		if(opt_type == OPT_BIG || opt_type == OPT_SMALL) {
			int delta_str_len;
			char tmp_varity_name[3];
			this->varity_declare->declare_analysis_varity(0, 0, tmp_varity_name, &tmp_varity);
			char name_buf[32];
			memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
			name_buf[symbol_pos_once] = 0;
			int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
			if(tmp_varity_type == 0) {
				finded_varity = (varity_info*)this->varity_declare->analysis_varity_stack->find(name_buf);
				if(!finded_varity)
					finded_varity = (varity_info*)this->varity_declare->global_varity_stack->find(name_buf);
				if(!finded_varity) {
					debug("varity \"%s\" doesn't exist\n", name_buf);
					return 1;
				}
				*tmp_varity = *finded_varity;
			} else if(tmp_varity_type == 1) {
				*tmp_varity = y_atof(name_buf);
			} else if(tmp_varity_type == 2) {
				*tmp_varity = y_atoi(name_buf);
			}
			symbol_pos_last += symbol_pos_once + opt_len;
			last_opt_type = opt_type;
			while((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) > 0) {
				size -= symbol_pos_once + opt_len;
				memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
				name_buf[symbol_pos_once] = 0;
				int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
				if(tmp_varity_type == 0) {
					finded_varity = (varity_info*)this->varity_declare->analysis_varity_stack->find(name_buf);
					if(!finded_varity)
						finded_varity = (varity_info*)this->varity_declare->global_varity_stack->find(name_buf);
					if(!finded_varity) {
						debug("varity \"%s\" doesn't exist\n", name_buf);
						return 1;
					}
					if(last_opt_type == OPT_BIG)
						*tmp_varity = *tmp_varity > *finded_varity;
					else if(last_opt_type == OPT_SMALL)
						*tmp_varity = *tmp_varity < *finded_varity;
				} else if(tmp_varity_type == 1) {
					varity_info sub_tmp_varity;
					sub_tmp_varity = y_atof(name_buf);
					if(last_opt_type == OPT_BIG)
						*tmp_varity = *tmp_varity > sub_tmp_varity;
					else if(last_opt_type == OPT_SMALL)
						*tmp_varity = *tmp_varity < sub_tmp_varity;
				} else if(tmp_varity_type == 2) {
					varity_info sub_tmp_varity;
					sub_tmp_varity = y_atoi(name_buf);
					if(last_opt_type == OPT_BIG)
						*tmp_varity = *tmp_varity > sub_tmp_varity;
					else if(last_opt_type == OPT_SMALL)
						*tmp_varity = *tmp_varity < sub_tmp_varity;
				}
				symbol_pos_last += symbol_pos_once + opt_len;
				if(opt_type != OPT_BIG && opt_type != OPT_SMALL)
					break;
			}
			delta_str_len = sub_replace(str, continuous_plus_begin_pos, symbol_pos_last - 1, tmp_varity_name);
			*size_ptr += delta_str_len;
			symbol_pos_last += delta_str_len;
			tmp_varity->echo();
		} else {
			symbol_pos_last += symbol_pos_once + opt_len;
		}
	}
	return 0;
}

int c_interpreter::bracket_opt(char* name, char* sub_sentence, char* ret_str, uint* ret_len)
{
	varity_info* varity_ptr, *index_varity_ptr, *ret_varity_ptr;
	int index;
	varity_ptr = this->varity_declare->find(name, PRODUCED_ALL);
	if(varity_ptr) {
		if(!(varity_ptr->get_size() != sizeof_type[varity_ptr->get_type()] || varity_ptr->get_type() >= sizeof(sizeof_type) / sizeof(sizeof_type[0])))
			return 1;
		int tmp_varity_type = check_symbol(sub_sentence, MAX_INT);
		if(tmp_varity_type == 0) {
			index_varity_ptr = this->varity_declare->find(sub_sentence, MAX_INT);
			if(!index_varity_ptr) {
				debug("varity \"%s\" doesn't exist\n");
				return 1;
			}
			if(index_varity_ptr->get_type() == FLOAT || index_varity_ptr->get_type() == DOUBLE) {
				debug("float can not used as index\n");
				return 1;
			}
			index = *(int*)index_varity_ptr->get_content_ptr();
		} else if(tmp_varity_type == 1) {
			debug("float can not used as index\n");
			return 1;
		} else if(tmp_varity_type == 2) {
			index = y_atoi(sub_sentence);
		}
		int ret_type = varity_ptr->get_type();
		this->varity_declare->declare_analysis_varity(ret_type, sizeof_type[ret_type], ret_str, &ret_varity_ptr);
		if(ret_type == DOUBLE) {
			*ret_varity_ptr = *(double*)((uint)varity_ptr->get_content_ptr() + sizeof_type[DOUBLE] * index);
		} else if(ret_type == FLOAT) {
			*ret_varity_ptr = *(float*)((uint)varity_ptr->get_content_ptr() + sizeof_type[FLOAT] * index);
		} else if(ret_type == U_LONG_LONG || ret_type == LONG_LONG) {
			*ret_varity_ptr = *(long long*)((uint)varity_ptr->get_content_ptr() + sizeof_type[FLOAT] * index);
		} else {
			*ret_varity_ptr = *(int*)((uint)varity_ptr->get_content_ptr() + sizeof_type[FLOAT] * index);
		}
	}
	*ret_len = strlen(ret_str);
	return 0;
}

/*varity_info c_interpreter::assign_opt(char* str, uint* len_ptr)
{
	varity_info ret;
	varity_info tmp_varity;
	varity_info* finded_varity;
	char tmpbuf[32];
	int len = *len_ptr;
	int i;
	int symbol_begin_pos;
	for(int num_flag=1, i=len-1, symbol_begin_pos=len; i>=-1; i--) {
		if(i == -1 || str[i] == '=' || str[i] == ',' || str[i] == ';') {
			memcpy(tmpbuf, str + i + 1, symbol_begin_pos - i - 1);
			tmpbuf[symbol_begin_pos - i - 1] = 0;
			if(num_flag == 1) {
				remove_substring(str, i + 1, symbol_begin_pos - 1);
				tmp_varity = atoi(tmpbuf);
			} else if(num_flag == 2) {
				remove_substring(str, i + 1, symbol_begin_pos - 1);
				tmp_varity = atof(tmpbuf);
			} else if(num_flag == 0) {
				finded_varity = (varity_info*)this->varity_declare->analysis_varity_stack->find(tmpbuf);
				if(!finded_varity)
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
}*/