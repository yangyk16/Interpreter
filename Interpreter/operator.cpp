#include "operator.h"
#include "interpreter.h"
#include "string_lib.h"
#include "varity.h"
#include "type.h"
#include "error.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//从右向左的运算符统统压栈，集中出栈处理。
static int operator_convert(char* str, int* opt_type_ptr, int opt_pos, int* opt_len_ptr)
{
	if(*opt_type_ptr == OPT_PLUS || *opt_type_ptr == OPT_MINUS) {
		if(!is_valid_c_char(str[opt_pos - 1])) {
			if(*opt_type_ptr == OPT_PLUS) {
				*opt_type_ptr = OPT_POSITIVE;
			} else {
				*opt_type_ptr = OPT_NEGATIVE;
			}
			return 1;
		} else {
			return 0;
		}
	} else if(*opt_type_ptr == OPT_PLUS_PLUS || *opt_type_ptr == OPT_MINUS_MINUS) {
		if(is_valid_c_char(str[opt_pos - 1]) && is_valid_c_char(str[opt_pos + *opt_len_ptr])) {
			if(*opt_type_ptr == OPT_PLUS_PLUS) {
				*opt_type_ptr = OPT_PLUS;
			} else {
				*opt_type_ptr = OPT_MINUS;
			}
			*opt_len_ptr = 1;
			return 1;
		}
	} else if(*opt_type_ptr == OPT_MUL || *opt_type_ptr == OPT_BIT_AND) {
		if(!is_valid_c_char(str[opt_pos - 1])) {
			if(*opt_type_ptr == OPT_MUL)
				*opt_type_ptr = OPT_PTR_CONTENT;
			else
				*opt_type_ptr = OPT_ADDRESS_OF;
			return 1;
		}
	}
	return 0;
}
int c_interpreter::member_opt(char* str, uint* size_ptr)
{
	varity_info *tmp_varity = 0, *finded_varity;
	uint size = *size_ptr;
	int opt_len = 0, opt_type, last_opt_type;
	int symbol_pos_last = 0, symbol_pos_once;
	while((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) > 0) {
		int continuous_plus_begin_pos = symbol_pos_last;
		int symbol_pos_cur = symbol_pos_last + symbol_pos_once + opt_len;
		size -= symbol_pos_once + opt_len;
		if(opt_type == OPT_REFERENCE || opt_type == OPT_MEMBER || opt_type == OPT_L_MID_BRACKET) {
			int delta_str_len;
			char tmp_varity_name[3];
			this->varity_declare->declare_analysis_varity(0, 0, tmp_varity_name, &tmp_varity);
			tmp_varity->config_varity(ATTRIBUTE_TYPE_UNFIXED | ATTRIBUTE_REFERENCE | ATTRIBUTE_LINK);
			char name_buf[32];
			memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
			name_buf[symbol_pos_once] = 0;
			int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
			if(tmp_varity_type == OPERAND_VARITY) {
				finded_varity = (varity_info*)this->varity_declare->find(name_buf, PRODUCED_ALL);
				if(!finded_varity) {
					error("Varity \"%s\" doesn't exist!\n", name_buf);
					return ERROR_VARITY_NONEXIST;
				}
				*tmp_varity = *finded_varity;
			} else {
				error("A constant cannot be used as a operand when using reference operator!\n");
				return ERROR_ILLEGAL_OPERAND;
			}
			symbol_pos_last += symbol_pos_once + opt_len;
			last_opt_type = opt_type;
			while((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) > 0) {
				size -= symbol_pos_once + opt_len;
				memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
				name_buf[symbol_pos_once] = 0;
				int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
				if(last_opt_type == OPT_MEMBER || last_opt_type == OPT_REFERENCE) {
					if(tmp_varity_type == OPERAND_MEMBER) {
						varity_info* member_varity_ptr;
						struct_info* struct_info_ptr = (struct_info*)tmp_varity->get_complex_ptr();
						if(last_opt_type == OPT_MEMBER && tmp_varity->get_type() != STRUCT) {
							error("Expression must contain struct!\n");
							return ERROR_ILLEGAL_OPERAND;
						}
						if(last_opt_type == OPT_REFERENCE) {
							if(tmp_varity->get_type() != STRUCT + BASIC_VARITY_TYPE_COUNT) {
								error("Expression must contain struct ptr!\n");
								return ERROR_ILLEGAL_OPERAND;
							}
							if(INT_VALUE(tmp_varity->get_content_ptr()) == NULL) {
								error("Using null ptr!\n");
								return ERROR_INVALID_OPERAND;
							}
						}
						member_varity_ptr = (varity_info*)struct_info_ptr->varity_stack_ptr->find(name_buf);
						if(!member_varity_ptr) {
							error("Member %s is not exist in struct!\n", name_buf);
							return ERROR_STRUCT_MEMBER;
						}
						if(last_opt_type == OPT_MEMBER) {
							*tmp_varity = struct_info_ptr->visit_struct_member(tmp_varity->get_content_ptr(), member_varity_ptr);
						} else if(last_opt_type == OPT_REFERENCE) {
							*tmp_varity = struct_info_ptr->visit_struct_member(*(void**)tmp_varity->get_content_ptr(), member_varity_ptr);
						}
					} else {
						error("A constant cannot be used as a operand when using reference operator!\n");
						return ERROR_ILLEGAL_OPERAND;
					}
				} else if(last_opt_type == OPT_L_MID_BRACKET) {
					int index;
					int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
					if(tmp_varity_type == OPERAND_VARITY) {
						finded_varity = (varity_info*)this->varity_declare->find(name_buf, PRODUCED_ALL);
						if(!finded_varity) {
							error("Varity \"%s\" doesn't exist!\n", name_buf);
							return ERROR_VARITY_NONEXIST;
						}
						index = *(int*)(finded_varity->get_content_ptr());
					} else if(tmp_varity_type == OPERAND_INTEGER) {
						index = y_atoi(name_buf);
					} else if(tmp_varity_type == OPERAND_FLOAT) {
						error("Float can not used as index!\n");
						return ERROR_FLOAT_USED_INDEX;
					} else
						return tmp_varity_type;
					tmp_varity->set_to_single(index);//TODO: 增加获取变量值整数值的函数，增加count字段
					//tmp_varity->set_content_ptr((char*)tmp_varity->get_content_ptr() + index * sizeof_type[tmp_varity->get_type()]);
					//str[symbol_pos_last + symbol_pos_once] = 0;
					//this->bracket_opt(tmp_varity->get_name(), str + symbol_pos_last, 0, 0);
				}
				symbol_pos_last += symbol_pos_once + opt_len;
				if(opt_type != OPT_REFERENCE && opt_type != OPT_MEMBER && opt_type != OPT_L_MID_BRACKET && opt_type != OPT_R_MID_BRACKET) {
					symbol_pos_last -= opt_len;
					break;
				}
				last_opt_type = opt_type;
			}
			delta_str_len = sub_replace(str, continuous_plus_begin_pos, symbol_pos_last - 1, tmp_varity_name);
			*size_ptr += delta_str_len;
			symbol_pos_last += delta_str_len + opt_len;
			tmp_varity->clear_attribute(ATTRIBUTE_REFERENCE | ATTRIBUTE_TYPE_UNFIXED);
			tmp_varity->echo();
		} else {
			symbol_pos_last += symbol_pos_once + opt_len;
		}
	}
	if(symbol_pos_once < -1)//operand error
		return symbol_pos_once;
	return ERROR_NO;
}

int c_interpreter::auto_inc_opt(char* str, uint* size_ptr)
{
	varity_info ret, *tmp_varity = 0;
	uint size = *size_ptr;
	varity_info* finded_varity;
	int opt_len = 0, opt_type, last_opt_type;
	int symbol_pos_last = 0, symbol_pos_once, continuous_plus_begin_pos;
	char opt_stack[32], stack_ptr = 0, symbol_stack_ptr = 0;
	varity_info type_convert_stack[16];
	int l_bracket_pos, convert_stack_ptr = 0;
	while((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) >= 0) {
		if(opt_type == OPT_PLUS || opt_type == OPT_MINUS || opt_type == OPT_PLUS_PLUS || opt_type == OPT_MINUS_MINUS || opt_type == OPT_NOT || opt_type == OPT_BIT_REVERT || opt_type == OPT_BIT_AND || opt_type == OPT_MUL || opt_type == OPT_L_SMALL_BRACKET || opt_type == OPT_R_SMALL_BRACKET) {
			int delta_str_len, is_convert;
			char tmp_varity_name[3], name_buf[32];;
			continuous_plus_begin_pos = symbol_pos_last;
			name_buf[0] = 0;
			is_convert = operator_convert(str, &opt_type, symbol_pos_last + symbol_pos_once, &opt_len);
			size -= symbol_pos_once + opt_len;
			if(opt_type != OPT_POSITIVE && opt_type != OPT_NEGATIVE && opt_type != OPT_PLUS_PLUS && opt_type != OPT_MINUS_MINUS && opt_type != OPT_NOT && opt_type != OPT_BIT_REVERT && opt_type != OPT_ADDRESS_OF && opt_type != OPT_PTR_CONTENT && opt_type != OPT_L_SMALL_BRACKET && opt_type != OPT_R_SMALL_BRACKET) {
				symbol_pos_last += symbol_pos_once + opt_len;
				continue;
			} else if(opt_type == OPT_L_SMALL_BRACKET) {
				l_bracket_pos = symbol_pos_last + symbol_pos_once;
			}
			if(symbol_pos_once) {
				symbol_stack_ptr = stack_ptr;
				memcpy(name_buf, str + symbol_pos_last, 32);
				name_buf[symbol_pos_once] = 0;
			}
			opt_stack[stack_ptr++] = opt_type;
			symbol_pos_last += symbol_pos_once + opt_len;
			this->varity_declare->declare_analysis_varity(0, 0, tmp_varity_name, &tmp_varity);
			tmp_varity->config_varity(ATTRIBUTE_TYPE_UNFIXED);
			while((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) >= 0) {
				is_convert = operator_convert(str, &opt_type, symbol_pos_last + symbol_pos_once, &opt_len);
				size -= symbol_pos_once + opt_len;
				if(symbol_pos_once && opt_type != OPT_R_SMALL_BRACKET) {
					if(name_buf[0] != 0) {
						error("operator error\n");
						return ERROR_OPERATOR;
					}
					symbol_stack_ptr = stack_ptr;
					memcpy(name_buf, str + symbol_pos_last, 32);
					name_buf[symbol_pos_once] = 0;
				}
				if(opt_type != OPT_POSITIVE && opt_type != OPT_NEGATIVE && opt_type != OPT_PLUS_PLUS && opt_type != OPT_MINUS_MINUS && opt_type != OPT_NOT && opt_type != OPT_BIT_REVERT && opt_type != OPT_ADDRESS_OF && opt_type != OPT_PTR_CONTENT && opt_type != OPT_L_SMALL_BRACKET && opt_type != OPT_R_SMALL_BRACKET) {
					symbol_pos_last += symbol_pos_once;
					break;
				} else if(opt_type == OPT_L_SMALL_BRACKET) {
					l_bracket_pos = symbol_pos_last + symbol_pos_once;
				} else if(opt_type == OPT_R_SMALL_BRACKET) {
					char type_str[32];
					memcpy(type_str, str + l_bracket_pos + 1, symbol_pos_last + symbol_pos_once - l_bracket_pos - 1);
					type_str[symbol_pos_last + symbol_pos_once - l_bracket_pos - 1] = 0;
					is_type_convert(type_str, &type_convert_stack[convert_stack_ptr++]);
				}
				opt_stack[stack_ptr++] = opt_type;
				symbol_pos_last += symbol_pos_once + opt_len;
			}
			int tmp_varity_type = check_symbol(name_buf, 32);
			if(tmp_varity_type == OPERAND_VARITY) {
				finded_varity = (varity_info*)this->varity_declare->find(name_buf, PRODUCED_ALL);
				if(!finded_varity) {
					error("Varity \"%s\" doesn't exist\n", name_buf);
					return ERROR_VARITY_NONEXIST;
				}
				*tmp_varity = *finded_varity;
			} else if(tmp_varity_type == OPERAND_FLOAT) {
				*tmp_varity = y_atof(name_buf);
			} else if(tmp_varity_type == OPERAND_INTEGER) {
				*tmp_varity = y_atoi(name_buf);
			} else
				return tmp_varity_type;
			while(stack_ptr--) {
				if(opt_stack[stack_ptr] == OPT_BIT_REVERT) {
					*tmp_varity = ~*tmp_varity;
				} else if(opt_stack[stack_ptr] == OPT_PLUS_PLUS) {
					if(stack_ptr < symbol_stack_ptr) {//前置++,TODO:避免++a=b的错误执行，即a赋值失败(考虑链接变量？)
						*tmp_varity = ++(*finded_varity);
					} else {
						*tmp_varity = (*finded_varity)++;
						tmp_varity->config_varity(ATTRIBUTE_RIGHT_VALUE);
					}
				} else if(opt_stack[stack_ptr] == OPT_MINUS_MINUS) {
					if(stack_ptr < symbol_stack_ptr) {//前置--
						*tmp_varity = --(*finded_varity);
					} else {
						*tmp_varity = (*finded_varity)--;
						tmp_varity->config_varity(ATTRIBUTE_RIGHT_VALUE);
					}
				} else if(opt_stack[stack_ptr] == OPT_NOT) {
					*tmp_varity = !*tmp_varity;
				} else if(opt_stack[stack_ptr] == OPT_NEGATIVE) {
					*tmp_varity = -*tmp_varity;
				} else if(opt_stack[stack_ptr] == OPT_ADDRESS_OF) {
					tmp_varity->set_type(finded_varity->get_type() + BASIC_VARITY_TYPE_COUNT);
					*tmp_varity = (int)finded_varity->get_content_ptr();
				} else if(opt_stack[stack_ptr] == OPT_PTR_CONTENT) {
					if(tmp_varity->get_type() < BASIC_VARITY_TYPE_COUNT) {
						error("There is no ptr varity exist!\n");
						return ERROR_ILLEGAL_OPERAND;				
					}else if(INT_VALUE(tmp_varity->get_content_ptr()) == NULL) {
						error("Using null ptr!\n");
						return ERROR_INVALID_OPERAND;
					}
					tmp_varity->config_varity(ATTRIBUTE_LINK);
					tmp_varity->set_type(tmp_varity->get_type() - BASIC_VARITY_TYPE_COUNT);
					tmp_varity->set_content_ptr((void*)INT_VALUE(tmp_varity->get_content_ptr()));
				} else if(opt_stack[stack_ptr] == OPT_L_SMALL_BRACKET) {
					int source_type = tmp_varity->get_type();
					tmp_varity->set_type(type_convert_stack[--convert_stack_ptr].get_type());
					tmp_varity->convert(tmp_varity->get_content_ptr(), source_type);
				}
			}
			delta_str_len = sub_replace(str, continuous_plus_begin_pos, symbol_pos_last - 1, tmp_varity_name);
			symbol_pos_last += delta_str_len + opt_len;
			*size_ptr += delta_str_len;
			tmp_varity->clear_attribute(ATTRIBUTE_TYPE_UNFIXED);
			//tmp_varity->config_varity(ATTRIBUTE_RIGHT_VALUE);
			tmp_varity->echo();
			//symbol_pos_last -= opt_len;
			stack_ptr = 0;
		} else {
			size -= symbol_pos_once + opt_len;
			symbol_pos_last += symbol_pos_once + opt_len;
		}
	}
	if(symbol_pos_once < -1)
		return symbol_pos_once;
	return ERROR_NO;
}

int c_interpreter::plus_opt(char* str, uint* size_ptr)
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
		if(opt_type == OPT_PLUS || opt_type == OPT_MINUS) {
			int delta_str_len;
			char tmp_varity_name[3];
			this->varity_declare->declare_analysis_varity(0, 0, tmp_varity_name, &tmp_varity);
			tmp_varity->config_varity(ATTRIBUTE_TYPE_UNFIXED);
			char name_buf[32];
			memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
			name_buf[symbol_pos_once] = 0;
			int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
			if(tmp_varity_type == OPERAND_VARITY) {
				finded_varity = (varity_info*)this->varity_declare->find(name_buf, PRODUCED_ALL);
				if(!finded_varity) {
					error("Varity \"%s\" doesn't exist\n", name_buf);
					return ERROR_VARITY_NONEXIST;
				}
				*tmp_varity = *finded_varity;
			} else if(tmp_varity_type == OPERAND_FLOAT) {
				*tmp_varity = y_atof(name_buf);
			} else if(tmp_varity_type == OPERAND_INTEGER) {
				*tmp_varity = y_atoi(name_buf);
			} else
				return tmp_varity_type;
			symbol_pos_last += symbol_pos_once + opt_len;
			last_opt_type = opt_type;
			while((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) > 0) {
				size -= symbol_pos_once + opt_len;
				memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
				name_buf[symbol_pos_once] = 0;
				int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
				if(tmp_varity_type == OPERAND_VARITY) {
					finded_varity = (varity_info*)this->varity_declare->find(name_buf, PRODUCED_ALL);
					if(!finded_varity) {
						error("Varity \"%s\" doesn't exist\n", name_buf);
						return ERROR_VARITY_NONEXIST;
					}
					if(last_opt_type == OPT_PLUS)
						*tmp_varity = *tmp_varity + *finded_varity;
					else if(last_opt_type == OPT_MINUS)
						*tmp_varity = *tmp_varity - *finded_varity;
				} else if(tmp_varity_type == OPERAND_FLOAT) {
					varity_info sub_tmp_varity;
					sub_tmp_varity = y_atof(name_buf);
					if(last_opt_type == OPT_PLUS)
						*tmp_varity = *tmp_varity + sub_tmp_varity;
					else if(last_opt_type == OPT_MINUS)
						*tmp_varity = *tmp_varity - sub_tmp_varity;
				} else if(tmp_varity_type == OPERAND_INTEGER) {
					varity_info sub_tmp_varity;
					sub_tmp_varity = y_atoi(name_buf);
					if(last_opt_type == OPT_PLUS)
						*tmp_varity = *tmp_varity + sub_tmp_varity;
					else if(last_opt_type == OPT_MINUS)
						*tmp_varity = *tmp_varity - sub_tmp_varity;
				} else
					return tmp_varity_type;
				symbol_pos_last += symbol_pos_once + opt_len;
				if(opt_type != OPT_PLUS && opt_type != OPT_MINUS) {
					symbol_pos_last -= opt_len;
					break;
				}
				last_opt_type = opt_type;
			}
			delta_str_len = sub_replace(str, continuous_plus_begin_pos, symbol_pos_last - 1, tmp_varity_name);
			*size_ptr += delta_str_len;
			symbol_pos_last += delta_str_len + opt_len; //跳过此次忽略的运算符，继续找后续的
			tmp_varity->clear_attribute(ATTRIBUTE_TYPE_UNFIXED);
			tmp_varity->config_varity(ATTRIBUTE_RIGHT_VALUE);
			tmp_varity->echo();
		} else {
			symbol_pos_last += symbol_pos_once + opt_len;
		}
	}
	if(symbol_pos_once < -1)
		return symbol_pos_once;
	return ERROR_NO;
}

int c_interpreter::multiply_opt(char* str, uint* size_ptr)
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
		if(opt_type == OPT_MUL || opt_type == OPT_DIVIDE || opt_type == OPT_MOD) {
			int delta_str_len;
			char tmp_varity_name[3];
			this->varity_declare->declare_analysis_varity(0, 0, tmp_varity_name, &tmp_varity);
			tmp_varity->config_varity(ATTRIBUTE_TYPE_UNFIXED);
			char name_buf[32];
			memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
			name_buf[symbol_pos_once] = 0;
			int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
			if(tmp_varity_type == OPERAND_VARITY) {
				finded_varity = (varity_info*)this->varity_declare->find(name_buf, PRODUCED_ALL);
				if(!finded_varity) {
					error("Varity \"%s\" doesn't exist\n", name_buf);
					return ERROR_VARITY_NONEXIST;
				}
				*tmp_varity = *finded_varity;
			} else if(tmp_varity_type == OPERAND_FLOAT) {
				*tmp_varity = y_atof(name_buf);
			} else if(tmp_varity_type == OPERAND_INTEGER) {
				*tmp_varity = y_atoi(name_buf);
			} else
				return tmp_varity_type;
			symbol_pos_last += symbol_pos_once + opt_len;
			last_opt_type = opt_type;
			while((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) > 0) {
				size -= symbol_pos_once + opt_len;
				memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
				name_buf[symbol_pos_once] = 0;
				int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
				if(tmp_varity_type == OPERAND_VARITY) {
					finded_varity = (varity_info*)this->varity_declare->find(name_buf, PRODUCED_ALL);
					if(!finded_varity) {
						error("Varity \"%s\" doesn't exist\n", name_buf);
						return ERROR_VARITY_NONEXIST;
					}
					if(last_opt_type == OPT_MUL)
						*tmp_varity = *tmp_varity * *finded_varity;
					else if(last_opt_type == OPT_DIVIDE)
						*tmp_varity = *tmp_varity / *finded_varity;
					else if(last_opt_type == OPT_MOD)
						*tmp_varity = *tmp_varity % *finded_varity;
				} else if(tmp_varity_type == OPERAND_FLOAT) {
					varity_info sub_tmp_varity;
					sub_tmp_varity = y_atof(name_buf);
					*tmp_varity = sub_tmp_varity * *tmp_varity;
				} else if(tmp_varity_type == OPERAND_INTEGER) {
					varity_info sub_tmp_varity;
					sub_tmp_varity = y_atoi(name_buf);
					if(last_opt_type == OPT_MUL)
						*tmp_varity = *tmp_varity * sub_tmp_varity;
					else if(last_opt_type == OPT_DIVIDE)
						*tmp_varity = *tmp_varity / sub_tmp_varity;
					else if(last_opt_type == OPT_MOD)
						*tmp_varity = *tmp_varity % sub_tmp_varity;
				} else
					return tmp_varity_type;
				symbol_pos_last += symbol_pos_once + opt_len;
				if(opt_type != OPT_MUL && opt_type != OPT_DIVIDE && opt_type != OPT_MOD) {
					symbol_pos_last -= opt_len;
					break;
				}
				last_opt_type = opt_type;
			}
			delta_str_len = sub_replace(str, continuous_plus_begin_pos, symbol_pos_last - 1, tmp_varity_name);
			*size_ptr += delta_str_len;
			symbol_pos_last += delta_str_len + opt_len;
			tmp_varity->clear_attribute(ATTRIBUTE_TYPE_UNFIXED);
			tmp_varity->config_varity(ATTRIBUTE_RIGHT_VALUE);
			tmp_varity->echo();
		} else {
			symbol_pos_last += symbol_pos_once + opt_len;
		}
	}
	if(symbol_pos_once < -1)
		return symbol_pos_once;
	return ERROR_NO;
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
			if(tmp_varity_type == OPERAND_VARITY) {
				finded_varity = (varity_info*)this->varity_declare->find(name_buf, PRODUCED_ALL);
				if(!finded_varity) {
					error("Varity \"%s\" doesn't exist\n", name_buf);
					return ERROR_VARITY_NONEXIST;
				}
				*tmp_varity = *finded_varity;
			} else if(tmp_varity_type == OPERAND_FLOAT) {
				*tmp_varity = y_atof(name_buf);
			} else if(tmp_varity_type == OPERAND_INTEGER) {
				*tmp_varity = y_atoi(name_buf);
			} else
				return tmp_varity_type;
			size = symbol_pos_cur + 1;
			while(size > 0) {
				symbol_pos_once = search_opt(str, size, 1, &opt_len, &opt_type);
				symbol_pos_cur = symbol_pos_once - 1;
				memcpy(name_buf, str + symbol_pos_once + opt_len, size - symbol_pos_once - opt_len);
				name_buf[size - symbol_pos_once - opt_len] = 0;
				int tmp_varity_type = check_symbol(name_buf, size - symbol_pos_once - opt_len);
				if(tmp_varity_type == OPERAND_VARITY) {
					finded_varity = (varity_info*)this->varity_declare->find(name_buf, PRODUCED_ALL);
					if(!finded_varity) {
						error("Varity \"%s\" doesn't exist\n", name_buf);
						return ERROR_VARITY_NONEXIST;
					}
					if(finded_varity->get_attribute() & ATTRIBUTE_RIGHT_VALUE) {
						error("Only left value can be assigned.\n");
						return ERROR_NEED_LEFT_VALUE;
					}
					*finded_varity = *tmp_varity;
					finded_varity->echo();
				} else if(tmp_varity_type == OPERAND_FLOAT) {
					//*tmp_varity = *tmp_varity;
					error("A constant cannot be assigned\n");
					return ERROR_CONST_ASSIGNED;
				} else if(tmp_varity_type == OPERAND_INTEGER) {
					//*tmp_varity = *tmp_varity;
					error("A constant cannot be assigned\n");
					return ERROR_CONST_ASSIGNED;
				} else
					return tmp_varity_type;
				size = symbol_pos_cur + 1;
				if(opt_type != OPT_ASSIGN && opt_type != OPT_ADD_ASSIGN)
					break;
			}
			memcpy((char*)tmp_varity + sizeof(element), (char*)finded_varity + sizeof(element), sizeof(varity_info) - sizeof(tmp_varity->get_name()));
			tmp_varity->config_varity(ATTRIBUTE_LINK);
			delta_str_len = sub_replace(str, symbol_pos_once + opt_len, continuous_assign_end_pos, tmp_varity_name);
			*len_ptr += delta_str_len;
			size = symbol_pos_cur + 1;
		} else {
			size = symbol_pos_cur + 1;
		}
	}
	if(symbol_pos_once < -1)
		return symbol_pos_once;
	return ERROR_NO;
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
		if(opt_type == OPT_BIG || opt_type == OPT_SMALL || opt_type == OPT_BIG_EQU || opt_type == OPT_SMALL_EQU) {
			int delta_str_len;
			char tmp_varity_name[3];
			this->varity_declare->declare_analysis_varity(0, 0, tmp_varity_name, &tmp_varity);
			tmp_varity->config_varity(ATTRIBUTE_TYPE_UNFIXED);
			char name_buf[32];
			memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
			name_buf[symbol_pos_once] = 0;
			int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
			if(tmp_varity_type == OPERAND_VARITY) {
				finded_varity = (varity_info*)this->varity_declare->find(name_buf, PRODUCED_ALL);
				if(!finded_varity) {
					error("Varity \"%s\" doesn't exist\n", name_buf);
					return ERROR_VARITY_NONEXIST;
				}
				*tmp_varity = *finded_varity;
			} else if(tmp_varity_type == OPERAND_FLOAT) {
				*tmp_varity = y_atof(name_buf);
			} else if(tmp_varity_type == OPERAND_INTEGER) {
				*tmp_varity = y_atoi(name_buf);
			} else
				return tmp_varity_type;
			symbol_pos_last += symbol_pos_once + opt_len;
			last_opt_type = opt_type;
			while((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) > 0) {
				size -= symbol_pos_once + opt_len;
				memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
				name_buf[symbol_pos_once] = 0;
				int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
				if(tmp_varity_type == OPERAND_VARITY) {
					finded_varity = (varity_info*)this->varity_declare->find(name_buf, PRODUCED_ALL);
					if(!finded_varity) {
						error("Varity \"%s\" doesn't exist\n", name_buf);
						return ERROR_VARITY_NONEXIST;
					}
					if(last_opt_type == OPT_BIG)
						*tmp_varity = *tmp_varity > *finded_varity;
					else if(last_opt_type == OPT_SMALL)
						*tmp_varity = *tmp_varity < *finded_varity;
					else if(last_opt_type == OPT_BIG_EQU)
						*tmp_varity = *tmp_varity >= *finded_varity;
					else if(last_opt_type == OPT_SMALL_EQU)
						*tmp_varity = *tmp_varity <= *finded_varity;
				} else if(tmp_varity_type == OPERAND_FLOAT) {
					varity_info sub_tmp_varity;
					sub_tmp_varity = y_atof(name_buf);
					if(last_opt_type == OPT_BIG)
						*tmp_varity = *tmp_varity > sub_tmp_varity;
					else if(last_opt_type == OPT_SMALL)
						*tmp_varity = *tmp_varity < sub_tmp_varity;
					else if(last_opt_type == OPT_BIG_EQU)
						*tmp_varity = *tmp_varity >= sub_tmp_varity;
					else if(last_opt_type == OPT_SMALL_EQU)
						*tmp_varity = *tmp_varity <= sub_tmp_varity;
				} else if(tmp_varity_type == OPERAND_INTEGER) {
					varity_info sub_tmp_varity;
					sub_tmp_varity = y_atoi(name_buf);
					if(last_opt_type == OPT_BIG)
						*tmp_varity = *tmp_varity > sub_tmp_varity;
					else if(last_opt_type == OPT_SMALL)
						*tmp_varity = *tmp_varity < sub_tmp_varity;
					else if(last_opt_type == OPT_BIG_EQU)
						*tmp_varity = *tmp_varity >= sub_tmp_varity;
					else if(last_opt_type == OPT_SMALL_EQU)
						*tmp_varity = *tmp_varity <= sub_tmp_varity;
				} else
					return tmp_varity_type;
				symbol_pos_last += symbol_pos_once + opt_len;
				if(opt_type != OPT_BIG && opt_type != OPT_SMALL && opt_type != OPT_BIG_EQU && opt_type != OPT_SMALL_EQU) {
					symbol_pos_last -= opt_len;
					break;
				}
				last_opt_type = opt_type;
			}
			delta_str_len = sub_replace(str, continuous_plus_begin_pos, symbol_pos_last - 1, tmp_varity_name);
			*size_ptr += delta_str_len;
			symbol_pos_last += delta_str_len + opt_len;
			tmp_varity->clear_attribute(ATTRIBUTE_TYPE_UNFIXED);
			tmp_varity->config_varity(ATTRIBUTE_RIGHT_VALUE);
			tmp_varity->echo();
		} else {
			symbol_pos_last += symbol_pos_once + opt_len;
		}
	}
	if(symbol_pos_once < -1)
		return symbol_pos_once;
	return ERROR_NO;
}

int c_interpreter::equal_opt(char* str, uint* size_ptr)
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
		if(opt_type == OPT_EQU || opt_type == OPT_NOT_EQU) {
			int delta_str_len;
			char tmp_varity_name[3];
			this->varity_declare->declare_analysis_varity(0, 0, tmp_varity_name, &tmp_varity);
			tmp_varity->config_varity(ATTRIBUTE_TYPE_UNFIXED);
			char name_buf[32];
			memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
			name_buf[symbol_pos_once] = 0;
			int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
			if(tmp_varity_type == OPERAND_VARITY) {
				finded_varity = (varity_info*)this->varity_declare->find(name_buf, PRODUCED_ALL);
				if(!finded_varity) {
					error("Varity \"%s\" doesn't exist\n", name_buf);
					return ERROR_VARITY_NONEXIST;
				}
				*tmp_varity = *finded_varity;
			} else if(tmp_varity_type == OPERAND_FLOAT) {
				*tmp_varity = y_atof(name_buf);
			} else if(tmp_varity_type == OPERAND_INTEGER) {
				*tmp_varity = y_atoi(name_buf);
			} else
				return tmp_varity_type;
			symbol_pos_last += symbol_pos_once + opt_len;
			last_opt_type = opt_type;
			while((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) > 0) {
				size -= symbol_pos_once + opt_len;
				memcpy(name_buf, str + symbol_pos_last, symbol_pos_once);
				name_buf[symbol_pos_once] = 0;
				int tmp_varity_type = check_symbol(name_buf, symbol_pos_once);
				if(tmp_varity_type == OPERAND_VARITY) {
					finded_varity = (varity_info*)this->varity_declare->find(name_buf, PRODUCED_ALL);
					if(!finded_varity) {
						error("Varity \"%s\" doesn't exist\n", name_buf);
						return ERROR_VARITY_NONEXIST;
					}
					if(last_opt_type == OPT_EQU)
						*tmp_varity = *tmp_varity == *finded_varity;
					else if(last_opt_type == OPT_NOT_EQU)
						*tmp_varity = *tmp_varity != *finded_varity;
				} else if(tmp_varity_type == OPERAND_FLOAT) {
					varity_info sub_tmp_varity;
					sub_tmp_varity = y_atof(name_buf);
					if(last_opt_type == OPT_EQU)
						*tmp_varity = *tmp_varity == sub_tmp_varity;
					else if(last_opt_type == OPT_NOT_EQU)
						*tmp_varity = *tmp_varity != sub_tmp_varity;
				} else if(tmp_varity_type == OPERAND_INTEGER) {
					varity_info sub_tmp_varity;
					sub_tmp_varity = y_atoi(name_buf);
					if(last_opt_type == OPT_EQU)
						*tmp_varity = *tmp_varity == sub_tmp_varity;
					else if(last_opt_type == OPT_NOT_EQU)
						*tmp_varity = *tmp_varity != sub_tmp_varity;
				} else
					return tmp_varity_type;
				symbol_pos_last += symbol_pos_once + opt_len;
				if(opt_type != OPT_EQU && opt_type != OPT_NOT_EQU) {
					symbol_pos_last -= opt_len;
					break;
				}
				last_opt_type = opt_type;
			}
			delta_str_len = sub_replace(str, continuous_plus_begin_pos, symbol_pos_last - 1, tmp_varity_name);
			*size_ptr += delta_str_len;
			symbol_pos_last += delta_str_len + opt_len;
			tmp_varity->clear_attribute(ATTRIBUTE_TYPE_UNFIXED);
			tmp_varity->config_varity(ATTRIBUTE_RIGHT_VALUE);
			tmp_varity->echo();
		} else {
			symbol_pos_last += symbol_pos_once + opt_len;
		}
	}
	if(symbol_pos_once < -1)
		return symbol_pos_once;
	return ERROR_NO;
}

typedef int (*opt_handle_func)(mid_code*);
opt_handle_func opt_handle[54];

int min(int a, int b){return a>b?b:a;}

int opt_mul_handle(mid_code* instruction_ptr)
{
	int *opda_addr, *opdb_addr;
	int ret_type;
	long long opda_value, opdb_value;
	switch(instruction_ptr->opda_operand_type) {
	case OPERAND_CONST:
		opda_addr = (int*)&opda_value;
		memcpy(opda_addr, &instruction_ptr->opda_addr, 8);
		break;
	case OPERAND_L_VARITY:
		opda_addr += 0;
	case OPERAND_T_VARITY:
	default:
		break;
	}
	switch(instruction_ptr->opdb_operand_type) {
	case OPERAND_CONST:
		opdb_addr = (int*)&opdb_value;
		memcpy(opdb_addr, &instruction_ptr->opdb_addr, 8);
		break;
	case OPERAND_L_VARITY:
	case OPERAND_T_VARITY:
	default:
		break;
	}
	ret_type = min(instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type);
	if(instruction_ptr->opda_varity_type == instruction_ptr->opdb_varity_type) {
		if(ret_type == U_LONG || ret_type == LONG || ret_type == U_INT || ret_type == INT) {
			*(int*)(instruction_ptr->ret_addr) = *(int*)(instruction_ptr->opda_addr) * *(int*)(instruction_ptr->opdb_addr);
		} else if(ret_type == U_SHORT || ret_type == SHORT) {
			*(short*)(instruction_ptr->ret_addr) = *(short*)(instruction_ptr->opda_addr) * *(short*)(instruction_ptr->opdb_addr);
		} else if(ret_type == U_CHAR || ret_type == CHAR) {
			*(char*)(instruction_ptr->ret_addr) = *(char*)(instruction_ptr->opda_addr) * *(char*)(instruction_ptr->opdb_addr);
		} else if(ret_type = DOUBLE) {
			*(double*)(instruction_ptr->ret_addr) = *(double*)(instruction_ptr->opda_addr) * *(double*)(instruction_ptr->opdb_addr);
		} else if(ret_type == FLOAT) {
			*(float*)(instruction_ptr->ret_addr) = *(float*)(instruction_ptr->opda_addr) * *(float*)(instruction_ptr->opdb_addr);
		}
	} else {
		int mintype = min(instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type);
		if(mintype == U_LONG || mintype == LONG || mintype == U_INT || mintype == INT) {
			*(int*)(instruction_ptr->ret_addr) = *(int*)(opda_addr) > *(int*)(opdb_addr);
		} else if(mintype == U_SHORT || mintype == SHORT) {
			*(int*)(instruction_ptr->ret_addr) = *(short*)(opda_addr) > *(short*)(opdb_addr);
		} else if(mintype == U_CHAR) {
			*(int*)(instruction_ptr->ret_addr) = *(char*)(opda_addr) > *(char*)(opdb_addr);
		} else if(mintype == DOUBLE) {
			if(instruction_ptr->opda_varity_type == DOUBLE) {
				if(instruction_ptr->opdb_varity_type == LONG || instruction_ptr->opdb_varity_type == INT || instruction_ptr->opdb_varity_type == SHORT || instruction_ptr->opdb_varity_type == CHAR) {
					*(int*)(instruction_ptr->ret_addr) = *(double*)(opda_addr) > (double)(*(int*)(opdb_addr));
				} else if(instruction_ptr->opdb_varity_type == U_LONG || instruction_ptr->opdb_varity_type == U_INT || instruction_ptr->opdb_varity_type == U_SHORT || instruction_ptr->opdb_varity_type == U_CHAR) {
					*(int*)(instruction_ptr->ret_addr) = *(double*)(opda_addr) > (double)(*(uint*)(opdb_addr));
				} else if(instruction_ptr->opdb_varity_type == FLOAT) {
					*(int*)(instruction_ptr->ret_addr) = *(double*)(opda_addr) > (double)(*(float*)(opdb_addr));
				}
			} else {
				if(instruction_ptr->opda_varity_type == LONG || instruction_ptr->opda_varity_type == INT || instruction_ptr->opda_varity_type == SHORT || instruction_ptr->opda_varity_type == CHAR) {
					*(int*)(instruction_ptr->ret_addr) = (double)(*(int*)(opda_addr)) > *(double*)(opdb_addr);
				} else if(instruction_ptr->opda_varity_type == U_LONG || instruction_ptr->opda_varity_type == U_INT || instruction_ptr->opda_varity_type == U_SHORT || instruction_ptr->opda_varity_type == U_CHAR) {
					*(int*)(instruction_ptr->ret_addr) = (double)(*(uint*)(opda_addr)) > *(double*)(opdb_addr);
				} else if(instruction_ptr->opda_varity_type == FLOAT) {
					*(int*)(instruction_ptr->ret_addr) = (double)(*(float*)(opda_addr)) > *(double*)(opdb_addr);
				}
			}
		} else if(mintype == FLOAT) {
			if(instruction_ptr->opda_varity_type == FLOAT) {
				if(instruction_ptr->opdb_varity_type == LONG || instruction_ptr->opdb_varity_type == INT || instruction_ptr->opdb_varity_type == SHORT || instruction_ptr->opdb_varity_type == CHAR) {
					*(int*)(instruction_ptr->ret_addr) = *(float*)(opda_addr) > (float)(*(int*)(opdb_addr));
				} else if(instruction_ptr->opdb_varity_type == U_LONG || instruction_ptr->opdb_varity_type == U_INT || instruction_ptr->opdb_varity_type == U_SHORT || instruction_ptr->opdb_varity_type == U_CHAR) {
					*(int*)(instruction_ptr->ret_addr) = *(float*)(opda_addr) > (float)(*(uint*)(opdb_addr));
				}
			} else {
				if(instruction_ptr->opda_varity_type == LONG || instruction_ptr->opda_varity_type == INT || instruction_ptr->opda_varity_type == SHORT || instruction_ptr->opda_varity_type == CHAR) {
					*(int*)(instruction_ptr->ret_addr) = (float)(*(int*)(opda_addr)) > *(float*)(opdb_addr);
				} else if(instruction_ptr->opda_varity_type == U_LONG || instruction_ptr->opda_varity_type == U_INT || instruction_ptr->opda_varity_type == U_SHORT || instruction_ptr->opda_varity_type == U_CHAR) {
					*(int*)(instruction_ptr->ret_addr) = (float)(*(uint*)(opda_addr)) > *(float*)(opdb_addr);
				}
			}
		}
	}
	return 0;
}

void handle_init(void)
{
	for(int i=0; i<54;i++)
		opt_handle[i] = 0;
	opt_handle[OPT_MUL] = opt_mul_handle;
}

int call_opt_handle(int opt, mid_code* instruction_ptr)
{
	if(opt_handle[opt])
		return opt_handle[opt](instruction_ptr);
	else
		return 0;
}