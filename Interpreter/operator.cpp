#include "operator.h"
#include "interpreter.h"
#include "string_lib.h"
#include "varity.h"
#include "type.h"
#include "error.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define OPERATOR_TYPE_NUM	128

typedef int (*func0)(void);
typedef int (*func1)(int);
typedef int (*func2)(int, int);
typedef int (*func3)(int, int, int);
typedef int (*func4)(int, int, int, int);
typedef int (*func5)(int, int, int, int, int);
typedef int (*func6)(int, int, int, int, int, int);
typedef int (*func7)(int, int, int, int, int, int, int);
typedef int (*func8)(int, int, int, int, int, int, int, int);
func0 func0_ptr;
func1 func1_ptr;
func2 func2_ptr;
func3 func3_ptr;
func4 func4_ptr;
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

#define BINARY_OPT_EXEC(x)	\
	mid_code *&instruction_ptr = interpreter_ptr->pc; \
	int ret_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr); \
	switch(ret_type) { \
	case U_LONG: \
	case LONG: \
	case U_INT: \
	case INT: \
		INT_VALUE(ret_addr) = INT_VALUE(opda_addr) x INT_VALUE(opdb_addr); \
		break; \
	case U_SHORT: \
	case SHORT: \
		SHORT_VALUE(ret_addr) = SHORT_VALUE(opda_addr) x SHORT_VALUE(opdb_addr); \
		break; \
	case U_CHAR: \
	case CHAR: \
		CHAR_VALUE(ret_addr) = CHAR_VALUE(opda_addr) x CHAR_VALUE(opdb_addr); \
		break; \
	case DOUBLE: \
		DOUBLE_VALUE(ret_addr) = DOUBLE_VALUE(opda_addr) x DOUBLE_VALUE(opdb_addr); \
		break; \
	case FLOAT: \
		FLOAT_VALUE(ret_addr) = FLOAT_VALUE(opda_addr) x FLOAT_VALUE(opdb_addr); \
	}

#define BINARY_RET_INT_OPT_EXEC(x)	\
	mid_code *&instruction_ptr = interpreter_ptr->pc; \
	int ret_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr); \
	switch(ret_type) { \
	case U_LONG: \
	case LONG: \
	case U_INT: \
	case INT: \
		INT_VALUE(ret_addr) = INT_VALUE(opda_addr) x INT_VALUE(opdb_addr); \
		break; \
	case U_SHORT: \
	case SHORT: \
		INT_VALUE(ret_addr) = SHORT_VALUE(opda_addr) x SHORT_VALUE(opdb_addr); \
		break; \
	case U_CHAR: \
	case CHAR: \
		INT_VALUE(ret_addr) = CHAR_VALUE(opda_addr) x CHAR_VALUE(opdb_addr); \
		break; \
	case DOUBLE: \
		INT_VALUE(ret_addr) = DOUBLE_VALUE(opda_addr) x DOUBLE_VALUE(opdb_addr); \
		break; \
	case FLOAT: \
		INT_VALUE(ret_addr) = FLOAT_VALUE(opda_addr) x FLOAT_VALUE(opdb_addr); \
	}

#define BINARY_ARG_INT_OPT_EXEC(x)	\
	mid_code *&instruction_ptr = interpreter_ptr->pc; \
	int ret_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr); \
	switch(ret_type) { \
	case U_LONG: \
	case LONG: \
	case U_INT: \
	case INT: \
		INT_VALUE(ret_addr) = INT_VALUE(opda_addr) x INT_VALUE(opdb_addr); \
		break; \
	case U_SHORT: \
	case SHORT: \
		INT_VALUE(ret_addr) = SHORT_VALUE(opda_addr) x SHORT_VALUE(opdb_addr); \
		break; \
	case U_CHAR: \
	case CHAR: \
		INT_VALUE(ret_addr) = CHAR_VALUE(opda_addr) x CHAR_VALUE(opdb_addr); \
		break; \
	}

#define UNARY_OPT_EXEC(x)	\
	mid_code *&instruction_ptr = interpreter_ptr->pc; \
	int ret_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr); \
	switch(ret_type) { \
	case U_LONG: \
	case LONG: \
	case U_INT: \
	case INT: \
		INT_VALUE(ret_addr) = x INT_VALUE(opdb_addr); \
		break; \
	case U_SHORT: \
	case SHORT: \
		INT_VALUE(ret_addr) = x SHORT_VALUE(opdb_addr); \
		break; \
	case U_CHAR: \
	case CHAR: \
		INT_VALUE(ret_addr) = x CHAR_VALUE(opdb_addr); \
	}/*核实是否全部单目运算符都使用int返回值*/

typedef int (*opt_handle_func)(c_interpreter*, int*, int*, int*);
opt_handle_func opt_handle[OPERATOR_TYPE_NUM];

int get_ret_type(int a, int b)
{
	if(a <= VOID && b <= VOID) {
		return a<b?b:a;
	} else if(a == STRUCT || b == STRUCT) {
		return ERROR_ILLEGAL_OPERAND;
	} else if(a >= PTR && b <= VOID || a <= VOID && b >= PTR) {
		return PTR;
	} else if(a >= PTR && b >= PTR) {
		return ERROR_ILLEGAL_OPERAND;
	}

}

int get_varity_size(int type)
{
	if(type <= CHAR)
		return sizeof_type[type];
	else
		return PLATFORM_WORD_LEN;
	return 0;
}

inline int varity_convert(void *converted_ptr, int converted_type, void *converting_ptr, int converting_type)
{
	if(converted_type >= PTR) {
		if(converting_type == INT || converting_type == U_INT || converting_type == LONG || converting_type == U_LONG) {
			INT_VALUE(converted_ptr) = INT_VALUE(converting_ptr);
		} else if(converting_type == CHAR || converting_type == U_CHAR) {
			INT_VALUE(converted_ptr) = CHAR_VALUE(converting_ptr);
		} else if(converting_type == SHORT || converting_type == U_SHORT) {
			INT_VALUE(converted_ptr) = SHORT_VALUE(converting_ptr);
		} else if(converting_type > CHAR) {
			INT_VALUE(converted_ptr) = INT_VALUE(converting_ptr);
		} else {
			error("There is no method for coverting ptr converting_type from float\n");
			return ERROR_TYPE_CONVERT;
		}
		return ERROR_NO;
	}
	if(converted_type == converting_type) {
		memcpy(converted_ptr, converting_ptr, get_varity_size(converting_type, 0, 0));//TODO:删除get_varity_size的简单重载
	} else if(converted_type > converting_type) {
		if(converted_type == INT || converted_type == U_INT || converted_type == LONG || converted_type == U_LONG) {
			if(converting_type == INT || converting_type == U_INT || converting_type == LONG || converting_type == U_LONG || converting_type == LONG_LONG || converting_type == U_LONG_LONG)
				INT_VALUE(converted_ptr) = INT_VALUE(converting_ptr);
			else if(converting_type == SHORT || converting_type == U_SHORT)
				INT_VALUE(converted_ptr) = SHORT_VALUE(converting_ptr);
			else if(converting_type == CHAR || converting_type == U_CHAR)
				INT_VALUE(converted_ptr) = CHAR_VALUE(converting_ptr);
		} else if(converted_type == U_SHORT || converted_type == SHORT) {
			if(converting_type == U_SHORT || converting_type == SHORT)
				SHORT_VALUE(converted_ptr) = SHORT_VALUE(converting_ptr);
			else if(converting_type == CHAR || converting_type == U_CHAR)
				SHORT_VALUE(converted_ptr) = CHAR_VALUE(converting_ptr);
		} else if(converted_type == U_CHAR) {
			CHAR_VALUE(converted_ptr) = CHAR_VALUE(converting_ptr);
		} else if(converted_type == DOUBLE) {
			switch(converting_type) {
			case U_CHAR:
				DOUBLE_VALUE(converted_ptr) = U_CHAR_VALUE(converting_ptr);
				break;
			case CHAR:
				DOUBLE_VALUE(converted_ptr) = CHAR_VALUE(converting_ptr);
				break;
			case U_SHORT:
				DOUBLE_VALUE(converted_ptr) = U_SHORT_VALUE(converting_ptr);
				break;
			case SHORT:
				DOUBLE_VALUE(converted_ptr) = SHORT_VALUE(converting_ptr);
				break;
			case U_INT:
				DOUBLE_VALUE(converted_ptr) = U_INT_VALUE(converting_ptr);
				break;
			case INT:
				DOUBLE_VALUE(converted_ptr) = INT_VALUE(converting_ptr);
				break;
			case U_LONG:
				DOUBLE_VALUE(converted_ptr) = U_LONG_VALUE(converting_ptr);
				break;
			case LONG:
				DOUBLE_VALUE(converted_ptr) = LONG_VALUE(converting_ptr);
				break;
			case FLOAT:
				DOUBLE_VALUE(converted_ptr) = FLOAT_VALUE(converting_ptr);
				break;
			case U_LONG_LONG:
				DOUBLE_VALUE(converted_ptr) = U_LONG_LONG_VALUE(converting_ptr);
				break;
			case LONG_LONG:
				DOUBLE_VALUE(converted_ptr) = LONG_LONG_VALUE(converting_ptr);
				break;
			}
		} else if(converted_type == FLOAT) {
			switch(converting_type) {
			case U_CHAR:
				FLOAT_VALUE(converted_ptr) = U_CHAR_VALUE(converting_ptr);
				break;
			case CHAR:
				FLOAT_VALUE(converted_ptr) = CHAR_VALUE(converting_ptr);
				break;
			case U_SHORT:
				FLOAT_VALUE(converted_ptr) = U_SHORT_VALUE(converting_ptr);
				break;
			case SHORT:
				FLOAT_VALUE(converted_ptr) = SHORT_VALUE(converting_ptr);
				break;
			case U_INT:
				FLOAT_VALUE(converted_ptr) = U_INT_VALUE(converting_ptr);
				break;
			case INT:
				FLOAT_VALUE(converted_ptr) = INT_VALUE(converting_ptr);
				break;
			case U_LONG:
				FLOAT_VALUE(converted_ptr) = U_LONG_VALUE(converting_ptr);
				break;
			case LONG:
				FLOAT_VALUE(converted_ptr) = LONG_VALUE(converting_ptr);
				break;
			case U_LONG_LONG:
				FLOAT_VALUE(converted_ptr) = U_LONG_LONG_VALUE(converting_ptr);
				break;
			case LONG_LONG:
				FLOAT_VALUE(converted_ptr) = LONG_LONG_VALUE(converting_ptr);
				break;
			}
		}
	} else if(converted_type < converting_type) {
		if(converted_type == FLOAT) {
			FLOAT_VALUE(converted_ptr) = (float)DOUBLE_VALUE(converting_ptr);
		} else if(converted_type == INT || converted_type == U_INT || converted_type == INT || converted_type == U_INT){
			if(converting_type == DOUBLE) {
				INT_VALUE(converted_ptr) = (int)DOUBLE_VALUE(converting_ptr);
			} else if(converting_type == FLOAT) {
				INT_VALUE(converted_ptr) = (int)FLOAT_VALUE(converting_ptr);
			} else {
				INT_VALUE(converted_ptr) = INT_VALUE(converting_ptr);
			}
		} else if(converted_type == U_SHORT || converted_type == SHORT) {
			if(converting_type == DOUBLE) {
				SHORT_VALUE(converted_ptr) = (short)DOUBLE_VALUE(converting_ptr);
			} else if(converting_type == FLOAT) {
				SHORT_VALUE(converted_ptr) = (short)FLOAT_VALUE(converting_ptr);
			} else {
				SHORT_VALUE(converted_ptr) = SHORT_VALUE(converting_ptr);
			}
		} else if(converted_type == U_CHAR || converted_type == CHAR) {
			if(converting_type == DOUBLE) {
				CHAR_VALUE(converted_ptr) = (char)DOUBLE_VALUE(converting_ptr);
			} else if(converting_type == FLOAT) {
				CHAR_VALUE(converted_ptr) = (char)FLOAT_VALUE(converting_ptr);
			} else {
				CHAR_VALUE(converted_ptr) = CHAR_VALUE(converting_ptr);
			}
		}
	}
	return ERROR_NO;
}

inline int exec_opt_preprocess(mid_code *instruction_ptr, int *&opda_addr, int *&opdb_addr)
{
	int converting_varity_type, ret_type;
	double converted_varity;
	void *converted_varity_ptr = &converted_varity, *converting_varity_ptr;
	ret_type = get_ret_type(instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type);
	if(instruction_ptr->opda_varity_type != instruction_ptr->opdb_varity_type && instruction_ptr->opda_varity_type < PTR) {
		if(instruction_ptr->opda_varity_type == ret_type) {
			converting_varity_ptr = (void*)opdb_addr;
			converting_varity_type = instruction_ptr->opdb_varity_type;
			opdb_addr = (int*)converted_varity_ptr;
		} else {
			converting_varity_ptr = (void*)opda_addr;
			converting_varity_type = instruction_ptr->opda_varity_type;
			opda_addr = (int*)converted_varity_ptr;
		}
		varity_convert(converted_varity_ptr, ret_type, converting_varity_ptr, converting_varity_type);
	}
	return ret_type;
}

int opt_asl_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	BINARY_ARG_INT_OPT_EXEC(<<);
	return ERROR_NO;
}

int opt_asr_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	BINARY_ARG_INT_OPT_EXEC(>>);
	return ERROR_NO;
}

int opt_big_equ_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	BINARY_RET_INT_OPT_EXEC(>=);
	return ERROR_NO;
}

int opt_small_equ_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	BINARY_RET_INT_OPT_EXEC(<=);
	return ERROR_NO;
}

int opt_equ_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	int ret_type, converting_varity_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	double converted_varity;
	void* converted_varity_ptr = &converted_varity, *converting_varity_ptr;
	ret_type = get_ret_type(instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type);
	if(instruction_ptr->opda_varity_type != instruction_ptr->opdb_varity_type) {
		if(instruction_ptr->opda_varity_type == ret_type) {
			converting_varity_ptr = (void*)opdb_addr;
			converting_varity_type = instruction_ptr->opdb_varity_type;
			opdb_addr = (int*)converted_varity_ptr;
		} else {
			converting_varity_ptr = (void*)opda_addr;
			converting_varity_type = instruction_ptr->opda_varity_type;
			opda_addr = (int*)converted_varity_ptr;
		}
		ret_type = varity_convert(converted_varity_ptr, ret_type, converting_varity_ptr, converting_varity_type);
		if(ret_type) return ret_type;
	}
	if(instruction_ptr->opda_varity_type == instruction_ptr->opdb_varity_type) {
		if(ret_type == U_LONG || ret_type == LONG || ret_type == U_INT || ret_type == INT) {
			INT_VALUE(ret_addr) = INT_VALUE(opda_addr) == INT_VALUE(opdb_addr);
		} else if(ret_type == U_SHORT || ret_type == SHORT) {
			INT_VALUE(ret_addr) = SHORT_VALUE(opda_addr) == SHORT_VALUE(opdb_addr);
		} else if(ret_type == U_CHAR || ret_type == CHAR) {
			INT_VALUE(ret_addr) = CHAR_VALUE(opda_addr) == CHAR_VALUE(opdb_addr);
		} else if(ret_type = DOUBLE) {
			INT_VALUE(ret_addr) = DOUBLE_VALUE(opda_addr) == DOUBLE_VALUE(opdb_addr);
		} else if(ret_type == FLOAT) {
			INT_VALUE(ret_addr) = FLOAT_VALUE(opda_addr) == FLOAT_VALUE(opdb_addr);
		}
	}
	return 0;
}

int opt_not_equ_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	BINARY_RET_INT_OPT_EXEC(!=);
	return ERROR_NO;
}

int opt_and_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	int ret;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	int converted_varitya, converted_varityb;
	ret = varity_convert(&converted_varitya, INT, opda_addr, instruction_ptr->opda_varity_type);
	ret |= varity_convert(&converted_varityb, INT, opdb_addr, instruction_ptr->opdb_varity_type);
	if(ret)
		return ERROR_TYPE_CONVERT;
	INT_VALUE(ret_addr) = converted_varitya && converted_varityb;
	return 0;
}

int opt_or_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	int ret;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	int converted_varitya, converted_varityb;
	ret = varity_convert(&converted_varitya, INT, opda_addr, instruction_ptr->opda_varity_type);
	ret |= varity_convert(&converted_varityb, INT, opdb_addr, instruction_ptr->opdb_varity_type);
	if(ret)
		return ERROR_TYPE_CONVERT;
	INT_VALUE(ret_addr) = converted_varitya || converted_varityb;
	return ERROR_NO;
}

int opt_member_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	INT_VALUE(ret_addr) = (int)opda_addr + INT_VALUE(opdb_addr);
	return ERROR_NO;
}

int opt_minus_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	int ret_type, converting_varity_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	double converted_varity;
	void* converted_varity_ptr = &converted_varity, *converting_varity_ptr;
	ret_type = get_ret_type(instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type);
	if(instruction_ptr->opda_varity_type != instruction_ptr->opdb_varity_type) {
		if(instruction_ptr->opda_varity_type == ret_type) {
			converting_varity_ptr = (void*)opdb_addr;
			converting_varity_type = instruction_ptr->opdb_varity_type;
			opdb_addr = (int*)converted_varity_ptr;
		} else {
			converting_varity_ptr = (void*)opda_addr;
			converting_varity_type = instruction_ptr->opda_varity_type;
			opda_addr = (int*)converted_varity_ptr;
		}
		ret_type = varity_convert(converted_varity_ptr, ret_type, converting_varity_ptr, converting_varity_type);
		if(ret_type) return ret_type;
	}
	if(ret_type == U_LONG || ret_type == LONG || ret_type == U_INT || ret_type == INT) {
		INT_VALUE(ret_addr) = INT_VALUE(opda_addr) - INT_VALUE(opdb_addr);
	} else if(ret_type == U_SHORT || ret_type == SHORT) {
		SHORT_VALUE(ret_addr) = SHORT_VALUE(opda_addr) - SHORT_VALUE(opdb_addr);
	} else if(ret_type == U_CHAR || ret_type == CHAR) {
		CHAR_VALUE(ret_addr) = CHAR_VALUE(opda_addr) - CHAR_VALUE(opdb_addr);
	} else if(ret_type == DOUBLE) {
		DOUBLE_VALUE(ret_addr) = DOUBLE_VALUE(opda_addr) - DOUBLE_VALUE(opdb_addr);
	} else if(ret_type == FLOAT) {
		FLOAT_VALUE(ret_addr) = FLOAT_VALUE(opda_addr) - FLOAT_VALUE(opdb_addr);
	}
	return ERROR_NO;
}

int opt_bit_revert_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	UNARY_OPT_EXEC(~);
	return ERROR_NO;
}

int opt_mul_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	int ret_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	ret_type = get_ret_type(instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type);
	if(instruction_ptr->opda_varity_type == instruction_ptr->opdb_varity_type) {
		if(ret_type == U_LONG || ret_type == LONG || ret_type == U_INT || ret_type == INT) {
			INT_VALUE(ret_addr) = INT_VALUE(opda_addr) * INT_VALUE(opdb_addr);
		} else if(ret_type == U_SHORT || ret_type == SHORT) {
			SHORT_VALUE(ret_addr) = SHORT_VALUE(opda_addr) * SHORT_VALUE(opdb_addr);
		} else if(ret_type == U_CHAR || ret_type == CHAR) {
			CHAR_VALUE(ret_addr) = CHAR_VALUE(opda_addr) * CHAR_VALUE(opdb_addr);
		} else if(ret_type = DOUBLE) {
			DOUBLE_VALUE(ret_addr) = DOUBLE_VALUE(opda_addr) * DOUBLE_VALUE(opdb_addr);
		} else if(ret_type == FLOAT) {
			FLOAT_VALUE(ret_addr) = FLOAT_VALUE(opda_addr) * FLOAT_VALUE(opdb_addr);
		}
	} else {
		if(ret_type == U_LONG || ret_type == LONG || ret_type == U_INT || ret_type == INT) {
			INT_VALUE(ret_addr) = INT_VALUE(opda_addr) * INT_VALUE(opdb_addr);
		} else if(ret_type == U_SHORT || ret_type == SHORT) {
			INT_VALUE(ret_addr) = SHORT_VALUE(opda_addr) * SHORT_VALUE(opdb_addr);
		} else if(ret_type == U_CHAR) {
			INT_VALUE(ret_addr) = CHAR_VALUE(opda_addr) * CHAR_VALUE(opdb_addr);
		} else if(ret_type == DOUBLE) {
			if(instruction_ptr->opda_varity_type == DOUBLE) {
				if(instruction_ptr->opdb_varity_type == LONG || instruction_ptr->opdb_varity_type == INT || instruction_ptr->opdb_varity_type == SHORT || instruction_ptr->opdb_varity_type == CHAR) {
					INT_VALUE(ret_addr) = DOUBLE_VALUE(opda_addr) * (double)(INT_VALUE(opdb_addr));
				} else if(instruction_ptr->opdb_varity_type == U_LONG || instruction_ptr->opdb_varity_type == U_INT || instruction_ptr->opdb_varity_type == U_SHORT || instruction_ptr->opdb_varity_type == U_CHAR) {
					INT_VALUE(ret_addr) = DOUBLE_VALUE(opda_addr) * (double)(U_INT_VALUE(opdb_addr));
				} else if(instruction_ptr->opdb_varity_type == FLOAT) {
					INT_VALUE(ret_addr) = DOUBLE_VALUE(opda_addr) * (double)(FLOAT_VALUE(opdb_addr));
				}
			} else {
				if(instruction_ptr->opda_varity_type == LONG || instruction_ptr->opda_varity_type == INT || instruction_ptr->opda_varity_type == SHORT || instruction_ptr->opda_varity_type == CHAR) {
					INT_VALUE(ret_addr) = (double)(INT_VALUE(opda_addr)) * DOUBLE_VALUE(opdb_addr);
				} else if(instruction_ptr->opda_varity_type == U_LONG || instruction_ptr->opda_varity_type == U_INT || instruction_ptr->opda_varity_type == U_SHORT || instruction_ptr->opda_varity_type == U_CHAR) {
					INT_VALUE(ret_addr) = (double)(U_INT_VALUE(opda_addr)) * DOUBLE_VALUE(opdb_addr);
				} else if(instruction_ptr->opda_varity_type == FLOAT) {
					INT_VALUE(ret_addr) = (double)(FLOAT_VALUE(opda_addr)) * DOUBLE_VALUE(opdb_addr);
				}
			}
		} else if(ret_type == FLOAT) {
			if(instruction_ptr->opda_varity_type == FLOAT) {
				if(instruction_ptr->opdb_varity_type == LONG || instruction_ptr->opdb_varity_type == INT || instruction_ptr->opdb_varity_type == SHORT || instruction_ptr->opdb_varity_type == CHAR) {
					INT_VALUE(ret_addr) = FLOAT_VALUE(opda_addr) * (float)(INT_VALUE(opdb_addr));
				} else if(instruction_ptr->opdb_varity_type == U_LONG || instruction_ptr->opdb_varity_type == U_INT || instruction_ptr->opdb_varity_type == U_SHORT || instruction_ptr->opdb_varity_type == U_CHAR) {
					INT_VALUE(ret_addr) = FLOAT_VALUE(opda_addr) * (float)(U_INT_VALUE(opdb_addr));
				}
			} else {
				if(instruction_ptr->opda_varity_type == LONG || instruction_ptr->opda_varity_type == INT || instruction_ptr->opda_varity_type == SHORT || instruction_ptr->opda_varity_type == CHAR) {
					INT_VALUE(ret_addr) = (float)(INT_VALUE(opda_addr)) * FLOAT_VALUE(opdb_addr);
				} else if(instruction_ptr->opda_varity_type == U_LONG || instruction_ptr->opda_varity_type == U_INT || instruction_ptr->opda_varity_type == U_SHORT || instruction_ptr->opda_varity_type == U_CHAR) {
					INT_VALUE(ret_addr) = (float)(U_INT_VALUE(opda_addr)) * FLOAT_VALUE(opdb_addr);
				}
			}
		}
	}
	return 0;
}

int opt_bit_and_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	BINARY_ARG_INT_OPT_EXEC(&);
	return ERROR_NO;
}

int opt_not_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	UNARY_OPT_EXEC(!);
	return ERROR_NO;
}

int opt_divide_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	BINARY_OPT_EXEC(/);
	return ERROR_NO;
}

int opt_mod_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	BINARY_ARG_INT_OPT_EXEC(%);
	return ERROR_NO;
}

int opt_plus_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	int ret_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	switch(ret_type) {
	case U_LONG:
	case LONG:
	case U_INT:
	case INT:
		INT_VALUE(ret_addr) = INT_VALUE(opda_addr) + INT_VALUE(opdb_addr);
		break;
	case U_SHORT:
	case SHORT:
		SHORT_VALUE(ret_addr) = SHORT_VALUE(opda_addr) + SHORT_VALUE(opdb_addr);
		break;
	case U_CHAR:
	case CHAR:
		CHAR_VALUE(ret_addr) = CHAR_VALUE(opda_addr) + CHAR_VALUE(opdb_addr);
		break;
	case DOUBLE:
		DOUBLE_VALUE(ret_addr) = DOUBLE_VALUE(opda_addr) + DOUBLE_VALUE(opdb_addr);
		break;
	case FLOAT:
		FLOAT_VALUE(ret_addr) = FLOAT_VALUE(opda_addr) + FLOAT_VALUE(opdb_addr);
		break;
	case PTR:
		if(instruction_ptr->opda_varity_type == PTR)
			PTR_N_VALUE(ret_addr) = PTR_N_VALUE(opda_addr) + PTR_N_VALUE(opdb_addr) * instruction_ptr->data;
		else 
			PTR_N_VALUE(ret_addr) = (PLATFORM_WORD)opda_addr + PTR_N_VALUE(opdb_addr) * instruction_ptr->data;
	}
	return ERROR_NO;
}

int opt_big_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	BINARY_RET_INT_OPT_EXEC(>);
	return ERROR_NO;
}

int opt_small_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	int ret_type, converting_varity_type, ret;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	double converted_varity;
	void* converted_varity_ptr = &converted_varity, *converting_varity_ptr;
	ret_type = INT;
	if(instruction_ptr->opda_varity_type != instruction_ptr->opdb_varity_type) {
		if(instruction_ptr->opda_varity_type == ret_type) {
			converting_varity_ptr = (void*)opdb_addr;
			converting_varity_type = instruction_ptr->opdb_varity_type;
			opdb_addr = (int*)converted_varity_ptr;
		} else {
			converting_varity_ptr = (void*)opda_addr;
			converting_varity_type = instruction_ptr->opda_varity_type;
			opda_addr = (int*)converted_varity_ptr;
		}
		ret = varity_convert(converted_varity_ptr, ret_type, converting_varity_ptr, converting_varity_type);
		if(ret) return ret;
	}
	if(ret_type == U_LONG || ret_type == LONG || ret_type == U_INT || ret_type == INT) {
		INT_VALUE(ret_addr) = INT_VALUE(opda_addr) < INT_VALUE(opdb_addr);
	} else if(ret_type == U_SHORT || ret_type == SHORT) {
		INT_VALUE(ret_addr) = SHORT_VALUE(opda_addr) < SHORT_VALUE(opdb_addr);
	} else if(ret_type == U_CHAR || ret_type == CHAR) {
		INT_VALUE(ret_addr) = CHAR_VALUE(opda_addr) < CHAR_VALUE(opdb_addr);
	} else if(ret_type == DOUBLE) {
		INT_VALUE(ret_addr) = DOUBLE_VALUE(opda_addr) < DOUBLE_VALUE(opdb_addr);
	} else if(ret_type == FLOAT) {
		INT_VALUE(ret_addr) = FLOAT_VALUE(opda_addr) < FLOAT_VALUE(opdb_addr);
	}
	return ERROR_NO;
}

int opt_bit_xor_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	BINARY_ARG_INT_OPT_EXEC(^);
	return ERROR_NO;
}

int opt_bit_or_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	BINARY_ARG_INT_OPT_EXEC(|);
	return ERROR_NO;
}

int opt_assign_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	//varity_convert(opda_addr, instruction_ptr->opda_varity_type, opdb_addr, instruction_ptr->opdb_varity_type);
	int type = instruction_ptr->opdb_varity_type;
	if(instruction_ptr->opda_varity_type == PTR) {
		if(type == ARRAY) {
			INT_VALUE(opda_addr) = (int)opdb_addr;
		} else if(type == INT || type == U_INT || type == LONG || type == U_LONG) {
			INT_VALUE(opda_addr) = INT_VALUE(opdb_addr);
		} else if(type == CHAR || type == U_CHAR) {
			INT_VALUE(opda_addr) = CHAR_VALUE(opdb_addr);
		} else if(type == SHORT || type == U_SHORT) {
			INT_VALUE(opda_addr) = SHORT_VALUE(opdb_addr);
		} else if(type > CHAR) {
			INT_VALUE(opda_addr) = INT_VALUE(opdb_addr);
		} else if(type == PTR) {
			PTR_VALUE(opda_addr) = PTR_VALUE(opdb_addr);
		}
		return 0;
	}
	if(instruction_ptr->opda_varity_type == type) {
		memcpy((void*)opda_addr, (void*)opdb_addr, sizeof_type[type]);
	} else if(instruction_ptr->opda_varity_type > type) {
		if(instruction_ptr->opda_varity_type == INT || instruction_ptr->opda_varity_type == U_INT || instruction_ptr->opda_varity_type == LONG || instruction_ptr->opda_varity_type == U_LONG)
			INT_VALUE(opda_addr) = INT_VALUE(opdb_addr);
		else if(instruction_ptr->opda_varity_type == U_SHORT || instruction_ptr->opda_varity_type == SHORT)
			SHORT_VALUE(opda_addr) = SHORT_VALUE(opdb_addr);
		else if(instruction_ptr->opda_varity_type == U_CHAR)
			CHAR_VALUE(opda_addr) = CHAR_VALUE(opdb_addr);
		else if(instruction_ptr->opda_varity_type == DOUBLE) {
			if(type == U_CHAR || type == U_INT || type == U_SHORT || type == U_LONG) {
				DOUBLE_VALUE(opda_addr) = U_INT_VALUE(opdb_addr);
			} else if(type == CHAR || type == INT || type == SHORT || type == LONG) {
				DOUBLE_VALUE(opda_addr) = INT_VALUE(opdb_addr);
			} else if(type == FLOAT) {
				DOUBLE_VALUE(opda_addr) = FLOAT_VALUE(opdb_addr);
			}
		} else if(instruction_ptr->opda_varity_type == FLOAT) {
			if(type == U_CHAR || type == U_INT || type == U_SHORT || type == U_LONG) {
				FLOAT_VALUE(opda_addr) = (float)U_INT_VALUE(opdb_addr);
			} else if(type == CHAR || type == INT || type == SHORT || type == LONG) {
				FLOAT_VALUE(opda_addr) = (float)INT_VALUE(opdb_addr);
			}
		}
	} else if(instruction_ptr->opda_varity_type < type) {
		if(type == PTR) {
			if(instruction_ptr->opda_varity_type == INT) {
				INT_VALUE(opda_addr) = INT_VALUE(opdb_addr);
			}
		} else if(type == ARRAY) {
			if(instruction_ptr->opda_varity_type == INT) {
				INT_VALUE(opda_addr) = int(opdb_addr);
			}
		} else if(type == DOUBLE) {
			INT_VALUE(opda_addr) = (int)DOUBLE_VALUE(opdb_addr);
		} else if(type == FLOAT) {
			INT_VALUE(opda_addr) = (int)FLOAT_VALUE(opdb_addr);
		} else if(type == INT || type == U_INT){
			INT_VALUE(opda_addr) = INT_VALUE(opdb_addr);
		} else if(instruction_ptr->opda_varity_type == U_SHORT || instruction_ptr->opda_varity_type == SHORT) {
			if(type == DOUBLE) {
				SHORT_VALUE(opda_addr) = (short)DOUBLE_VALUE(opdb_addr);
			} else if(type == FLOAT) {
				SHORT_VALUE(opda_addr) = (short)FLOAT_VALUE(opdb_addr);
			} else {
				SHORT_VALUE(opda_addr) = SHORT_VALUE(opdb_addr);
			}
		} else if(instruction_ptr->opda_varity_type == U_CHAR || instruction_ptr->opda_varity_type == CHAR) {
			if(type == DOUBLE) {
				CHAR_VALUE(opda_addr) = (char)DOUBLE_VALUE(opdb_addr);
			} else if(type == FLOAT) {
				CHAR_VALUE(opda_addr) = (char)FLOAT_VALUE(opdb_addr);
			} else {
				CHAR_VALUE(opda_addr) = CHAR_VALUE(opdb_addr);
			}
		}
	}
	return 0;
}

int opt_negative_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	int ret_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	switch(ret_type) {
	case U_LONG:
	case LONG:
	case U_INT:
	case INT:
		INT_VALUE(ret_addr) = - INT_VALUE(opdb_addr);
		break;
	case U_SHORT:
	case SHORT:
		SHORT_VALUE(ret_addr) = - SHORT_VALUE(opdb_addr);
		break;
	case U_CHAR:
	case CHAR:
		CHAR_VALUE(ret_addr) = - CHAR_VALUE(opdb_addr);
	case DOUBLE:
		DOUBLE_VALUE(ret_addr) = - DOUBLE_VALUE(opdb_addr);
	case FLOAT:
		FLOAT_VALUE(ret_addr) = - FLOAT_VALUE(opdb_addr);
	}
	return ERROR_NO;
}

int opt_positive_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	return ERROR_NO;
}

int opt_ptr_content_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	if(instruction_ptr->opdb_varity_type == ARRAY)
		PTR_VALUE(ret_addr) = opdb_addr;//生成链接变量
	else if(instruction_ptr->opdb_varity_type == PTR) {
		PTR_VALUE(ret_addr) = PTR_VALUE(opdb_addr);
	}
	//varity_convert(ret_addr, PLATFORM_TYPE, PTR_VALUE(opdb_addr), PLATFORM_TYPE);
	return ERROR_NO;
}

int opt_address_of_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	PTR_VALUE(ret_addr) = opdb_addr;//生成中间变量
	return ERROR_NO;
}

int opt_index_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	if(instruction_ptr->opda_varity_type == ARRAY)
		INT_VALUE(ret_addr) = (int)opda_addr + INT_VALUE(opdb_addr) * instruction_ptr->data;
	else if(instruction_ptr->opda_varity_type == PTR)
		INT_VALUE(ret_addr) = INT_VALUE(opda_addr) + INT_VALUE(opdb_addr) * instruction_ptr->data;
	return ERROR_NO;
}

int opt_call_func_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	mid_code *instruction_ptr = interpreter_ptr->pc, *pc_backup = interpreter_ptr->pc;
	function_info *function_ptr = (function_info*)opda_addr;
	int code_count = function_ptr->mid_code_stack.get_count();
	if(interpreter_ptr->call_func_info.function_depth == 0)
		interpreter_ptr->stack_pointer += interpreter_ptr->nonseq_info->stack_frame_size;
	else
		interpreter_ptr->stack_pointer += interpreter_ptr->call_func_info.cur_stack_frame_size[interpreter_ptr->call_func_info.function_depth - 1];
	interpreter_ptr->tmp_varity_stack_pointer += (int)opdb_addr;//24;//一句产生的中间代码可能最多用三个临时变量吧…TODO:考虑link变量后在CALL_FUNC生成代码时附加当前使用get_count()值？
	interpreter_ptr->call_func_info.cur_stack_frame_size[interpreter_ptr->call_func_info.function_depth] = function_ptr->stack_frame_size;
	interpreter_ptr->call_func_info.function_depth++;
	if(!function_ptr->compile_func_flag) {
		interpreter_ptr->exec_mid_code((mid_code*)function_ptr->mid_code_stack.get_base_addr(), code_count);
		varity_convert(ret_addr, instruction_ptr->ret_varity_type, interpreter_ptr->tmp_varity_stack_pointer, ((varity_info*)function_ptr->arg_list->visit_element_by_index(0))->get_type());
	} else {
		int ret;
		PLATFORM_WORD *arg_ptr = (PLATFORM_WORD*)interpreter_ptr->stack_pointer;
		switch(instruction_ptr->data) {
		case 1:
			func1_ptr = (func1)function_ptr->func_addr;
			ret = func1_ptr(*arg_ptr);
			break;
		case 2:
			func2_ptr = (func2)function_ptr->func_addr;
			ret = func2_ptr(*arg_ptr, *(arg_ptr + 1));
			break;
		case 3:
			func3_ptr = (func3)function_ptr->func_addr;
			ret = func3_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2));
		case 4:
			func4_ptr = (func4)function_ptr->func_addr;
			ret = func4_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2), *(arg_ptr + 3));
		case 16:
			break;
		}
	}
	interpreter_ptr->pc = pc_backup;
	
	interpreter_ptr->call_func_info.function_depth--;
	interpreter_ptr->tmp_varity_stack_pointer -= (int)opdb_addr;//24;
	if(interpreter_ptr->call_func_info.function_depth == 0)
		interpreter_ptr->stack_pointer -= interpreter_ptr->nonseq_info->stack_frame_size;
	else
		interpreter_ptr->stack_pointer -= interpreter_ptr->call_func_info.cur_stack_frame_size[interpreter_ptr->call_func_info.function_depth - 1];
	return 0;
}

int opt_func_comma_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)//TODO: 可删，根本不会生成comma中间代码，生成的是passpara
{
	return opt_assign_handle(interpreter_ptr, opda_addr, opdb_addr, ret_addr);
}

static int* last_ret_abs_addr;
int ctl_branch_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	instruction_ptr += instruction_ptr->opda_addr - 1;
	return ERROR_NO;
}

int ctl_branch_true_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	mid_code *last_instruction_ptr = instruction_ptr - 1;
	if(last_instruction_ptr->ret_varity_type != INT)//TODO:加入其他合法类型
		return ERROR_CONDITION_TYPE;
	if(is_non_zero(last_instruction_ptr->ret_varity_type, last_ret_abs_addr))
		instruction_ptr += instruction_ptr->opda_addr - 1;
	return ERROR_NO;
}

int ctl_branch_false_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	mid_code *last_instruction_ptr = instruction_ptr - 1;
	if(last_instruction_ptr->ret_varity_type != INT)
		return ERROR_CONDITION_TYPE;
	if(!is_non_zero(last_instruction_ptr->ret_varity_type, last_ret_abs_addr))
		instruction_ptr += instruction_ptr->opda_addr - 1;
	return ERROR_NO;
}

int opt_pass_para_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	return opt_assign_handle(interpreter_ptr, opda_addr, opdb_addr, ret_addr);
}

int ctl_return_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	instruction_ptr += instruction_ptr->opda_addr - 1;
	return ERROR_NO;
}

int sys_stack_step_handle(c_interpreter *interpreter_ptr, int *opda_addr, int *opdb_addr, int *ret_addr)
{
	interpreter_ptr->stack_pointer += (int)opda_addr;
	return ERROR_NO;
}

void handle_init(void)
{
	for(int i=0; i<OPERATOR_TYPE_NUM;i++)
		opt_handle[i] = 0;
	opt_handle[OPT_ASL] = opt_asl_handle;
	opt_handle[OPT_ASR] = opt_asr_handle;
	opt_handle[OPT_BIG_EQU] = opt_big_equ_handle;
	opt_handle[OPT_SMALL_EQU] = opt_small_equ_handle;
	opt_handle[OPT_EQU] = opt_equ_handle;
	opt_handle[OPT_NOT_EQU] = opt_not_equ_handle;
	opt_handle[OPT_AND] = opt_and_handle;
	opt_handle[OPT_OR] = opt_or_handle;
	opt_handle[OPT_MEMBER] = opt_member_handle;
	opt_handle[OPT_MINUS] = opt_minus_handle;
	opt_handle[OPT_BIT_REVERT] = opt_bit_revert_handle;
	opt_handle[OPT_MUL] = opt_mul_handle;
	opt_handle[OPT_BIT_AND] = opt_bit_and_handle;
	opt_handle[OPT_NOT] = opt_not_handle;
	opt_handle[OPT_DIVIDE] = opt_divide_handle;
	opt_handle[OPT_MOD] = opt_mod_handle;
	opt_handle[OPT_PLUS] = opt_plus_handle;
	opt_handle[OPT_BIG] = opt_big_handle;
	opt_handle[OPT_SMALL] = opt_small_handle;
	opt_handle[OPT_BIT_XOR] = opt_bit_xor_handle;
	opt_handle[OPT_BIT_OR] = opt_bit_or_handle;
	opt_handle[OPT_ASSIGN] = opt_assign_handle;
	opt_handle[OPT_NEGATIVE] = opt_negative_handle;
	opt_handle[OPT_POSITIVE] = opt_positive_handle;
	opt_handle[OPT_PTR_CONTENT] = opt_ptr_content_handle;
	opt_handle[OPT_ADDRESS_OF] = opt_address_of_handle;
	opt_handle[OPT_INDEX] = opt_index_handle;
	opt_handle[OPT_CALL_FUNC] = opt_call_func_handle;
	opt_handle[OPT_FUNC_COMMA] = opt_func_comma_handle;
	opt_handle[CTL_BRANCH] = ctl_branch_handle;
	opt_handle[CTL_BRANCH_TRUE] = ctl_branch_true_handle;
	opt_handle[CTL_BRANCH_FALSE] = ctl_branch_false_handle;
	opt_handle[OPT_PASS_PARA] = opt_pass_para_handle;
	opt_handle[CTL_RETURN] = ctl_return_handle;
	opt_handle[CTL_BREAK] = ctl_branch_handle;
	opt_handle[CTL_CONTINUE] = ctl_branch_handle;
	opt_handle[CTL_GOTO] = ctl_branch_handle;
	opt_handle[SYS_STACK_STEP] = sys_stack_step_handle;
}

int call_opt_handle(c_interpreter *interpreter_ptr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	int *opda_addr, *opdb_addr, *ret_addr, ret;
	long long opda_value, opdb_value;
	switch(instruction_ptr->opda_operand_type) {
	case OPERAND_CONST:
		opda_addr = (int*)&opda_value;
		memcpy(opda_addr, &instruction_ptr->opda_addr, 8);
		break;
	case OPERAND_L_VARITY:
		opda_addr = (int*)(sp + instruction_ptr->opda_addr);
		break;
	case OPERAND_T_VARITY:
		opda_addr = (int*)(t_varity_sp + instruction_ptr->opda_addr);
		break;
	case OPERAND_L_S_VARITY:
		if(interpreter_ptr->call_func_info.function_depth == 0)
			opda_addr = (int*)(interpreter_ptr->nonseq_info->stack_frame_size + instruction_ptr->opda_addr + sp);
		else
			opda_addr = (int*)(interpreter_ptr->call_func_info.cur_stack_frame_size[interpreter_ptr->call_func_info.function_depth - 1] + instruction_ptr->opda_addr + sp);
		break;
	case OPERAND_LINK_VARITY:
		opda_addr = (int*)PTR_VALUE(t_varity_sp + instruction_ptr->opda_addr);
		break;
	default:
		opda_addr = (int*)instruction_ptr->opda_addr;
		break;
	}
		
	switch(instruction_ptr->opdb_operand_type) {
	case OPERAND_CONST:
		opdb_addr = (int*)&opdb_value;
		memcpy(opdb_addr, &instruction_ptr->opdb_addr, 8);
		break;
	case OPERAND_L_VARITY:
		opdb_addr = (int*)(sp + instruction_ptr->opdb_addr);
		break;
	case OPERAND_T_VARITY:
		opdb_addr = (int*)(t_varity_sp + instruction_ptr->opdb_addr);
		break;
	case OPERAND_L_S_VARITY:
		if(interpreter_ptr->call_func_info.function_depth == 0)
			opdb_addr = (int*)(interpreter_ptr->nonseq_info->stack_frame_size + instruction_ptr->opdb_addr + sp);
		else
			opdb_addr = (int*)(interpreter_ptr->call_func_info.cur_stack_frame_size[interpreter_ptr->call_func_info.function_depth - 1] + instruction_ptr->opdb_addr + sp);
		break;
	case OPERAND_LINK_VARITY:
		opdb_addr = (int*)PTR_VALUE(t_varity_sp + instruction_ptr->opdb_addr);
		break;
	default:
		opdb_addr = (int*)instruction_ptr->opdb_addr;
		break;
	}
	switch(instruction_ptr->ret_operand_type) {
	case OPERAND_L_VARITY:
		ret_addr = (int*)(instruction_ptr->ret_addr + sp);
		break;
	case OPERAND_T_VARITY:
		ret_addr = (int*)(instruction_ptr->ret_addr + t_varity_sp);
		break;
	case OPERAND_LINK_VARITY:
		ret_addr = (int*)(t_varity_sp + instruction_ptr->ret_addr);
		break;
	default:
		ret_addr = (int*)instruction_ptr->ret_addr;
		break;
	}
	if(opt_handle[instruction_ptr->ret_operator])
		ret = opt_handle[instruction_ptr->ret_operator](interpreter_ptr, opda_addr, opdb_addr, ret_addr);
	else {
		error("no handle for operator %d\n", instruction_ptr->ret_operator);
		ret = 0;
	}
	last_ret_abs_addr = ret_addr;
	return ret;
}

int try_assign_handle(int opda_type, int opdb_type, int opda_complex_count, int *opda_type_info, int opdb_complex_count, int *opdb_type_info)
{
	if(opda_type == PTR) {
		if(opdb_type != PTR && opdb_type != ARRAY)
			goto wrong;
		else {
			if(opda_complex_count == 2 && opda_type_info[1] == BASIC_TYPE_SET(VOID) && GET_COMPLEX_TYPE(opda_type_info[2]) == COMPLEX_PTR)
				return ERROR_NO;
			else {
				if(opda_complex_count != opdb_complex_count)
					goto wrong;
				else {
					for(int i=1; i<=opda_complex_count-1; i++) {
						if(opda_type_info[i] != opdb_type_info[i])
							goto wrong;
					}
				}
			}
		}
	} else {
		if(opdb_type < CHAR && opdb_type > DOUBLE)
			goto wrong;
	}
	return ERROR_NO;
wrong:
	error("Can't assign.\n");
	return ERROR_ILLEGAL_OPERAND;
}

int try_plus_handle(int opda_type, int opdb_type, int opda_complex_count, int *opda_type_info, int opdb_complex_count, int *opdb_type_info)
{
	if(opda_type < VOID && opdb_type < VOID) {
		return opda_type<opdb_type?opdb_type:opda_type;
	} else if(opda_type == STRUCT || opdb_type == STRUCT || opda_type == VOID || opdb_type == VOID) {
		return ERROR_ILLEGAL_OPERAND;
	} else if(opda_type >= PTR && opdb_type < VOID || opda_type < VOID && opdb_type >= PTR) {
		return PTR;
	} else if(opda_type >= PTR && opdb_type >= PTR) {
		return ERROR_ILLEGAL_OPERAND;
	}
}

int try_mul_handle(int opda_type, int opdb_type, int opda_complex_count, int *opda_type_info, int opdb_complex_count, int *opdb_type_info)
{
	if(opda_type > DOUBLE || opdb_type > DOUBLE)
		return ERROR_ILLEGAL_OPERAND;
	return opda_type<opdb_type?opdb_type:opda_type;
}

int try_compare_handle(int opda_type, int opdb_type, int opda_complex_count, int *opda_type_info, int opdb_complex_count, int *opdb_type_info)
{
	if(opda_type > STRUCT) {
		if(opdb_type <= STRUCT)
			goto wrong;
		else {
			if(opda_complex_count != opdb_complex_count)
				goto wrong;
			else {
				for(int i=1; i<=opda_complex_count-1; i++) {
					if(opda_type_info[i] != opdb_type_info[i])
						goto wrong;
				}
			}
		}
	} else if(opda_type == STRUCT || opdb_type == STRUCT)
		goto wrong;
	return ERROR_NO;
wrong:
	error("Can't compare.\n");
	return ERROR_ILLEGAL_OPERAND;
}

int try_mod_handle(int opda_type, int opdb_type, int opda_complex_count, int *opda_type_info, int opdb_complex_count, int *opdb_type_info)
{
	if(opda_type > U_LONG_LONG || opdb_type > U_LONG_LONG)
		goto wrong;
	return ERROR_NO;
wrong:
	error("Can't mod.\n");
	return ERROR_ILLEGAL_OPERAND;
}

int try_call_opt_handle(int opt, int opda_type, int opdb_type, int opda_complex_count, int *opda_type_info, int opdb_complex_count, int *opdb_type_info)
{
	if(opt == OPT_ASSIGN)
		return try_assign_handle(opda_type, opdb_type, opda_complex_count, opda_type_info, opdb_complex_count, opdb_type_info);
	else if(opt == OPT_PLUS)
		return try_plus_handle(opda_type, opdb_type, opda_complex_count, opda_type_info, opdb_complex_count, opdb_type_info);
	else if(opt == OPT_MUL)
		return try_mul_handle(opda_type, opdb_type, opda_complex_count, opda_type_info, opdb_complex_count, opdb_type_info);
	else if(opt == OPT_EQU)
		return try_compare_handle(opda_type, opdb_type, opda_complex_count, opda_type_info, opdb_complex_count, opdb_type_info);
	else if(opt == OPT_MOD)
		return try_mod_handle(opda_type, opdb_type, opda_complex_count, opda_type_info, opdb_complex_count, opdb_type_info);
	return ERROR_NO;
}