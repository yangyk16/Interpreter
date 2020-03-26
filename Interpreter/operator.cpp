#include "operator.h"
#include "interpreter.h"
#include "string_lib.h"
#include "varity.h"
#include "type.h"
#include "error.h"
#include "cstdlib.h"
#if DEBUG_EN
#include "gdb.h"
#endif

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
typedef int (*func9)(int, int, int, int, int, int, int, int, int);
typedef int (*func10)(int, int, int, int, int, int, int, int, int, int);
typedef int (*func11)(int, int, int, int, int, int, int, int, int, int, int);
typedef int (*func12)(int, int, int, int, int, int, int, int, int, int, int, int);
typedef int (*func13)(int, int, int, int, int, int, int, int, int, int, int, int, int);
typedef int (*func14)(int, int, int, int, int, int, int, int, int, int, int, int, int, int);
typedef int (*func15)(int, int, int, int, int, int, int, int, int, int, int, int, int, int, int);
typedef int (*func16)(int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int);
func0 func0_ptr;
func1 func1_ptr;
func2 func2_ptr;
func3 func3_ptr;
func4 func4_ptr;
func5 func5_ptr;
func6 func6_ptr;
func7 func7_ptr;
func8 func8_ptr;
func9 func9_ptr;
func10 func10_ptr;
func11 func11_ptr;
func12 func12_ptr;
func13 func13_ptr;
func14 func14_ptr;
func15 func15_ptr;
func16 func16_ptr;

#define GET_OPDA_ADDR_NOCHECK() \
	int *opda_addr; \
	long long opda_value; \
	switch(instruction_ptr->opda_operand_type) { \
	case OPERAND_CONST: \
		opda_addr = (int*)&opda_value; \
		LONG_LONG_VALUE(opda_addr) = LONG_LONG_VALUE(&instruction_ptr->opda_addr); \
		break; \
	case OPERAND_L_VARITY: \
		opda_addr = (int*)(sp + instruction_ptr->opda_addr); \
		break; \
	case OPERAND_T_VARITY: \
		opda_addr = (int*)(t_varity_sp - instruction_ptr->opda_addr - 8); \
		break; \
	case OPERAND_LINK_VARITY: \
		opda_addr = (int*)PTR_VALUE(t_varity_sp - instruction_ptr->opda_addr - 8); \
		break; \
	default: \
		opda_addr = (int*)instruction_ptr->opda_addr; \
		break; \
	}

#define GET_OPDB_ADDR_NOCHECK() \
	int *opdb_addr; \
	long long opdb_value; \
	switch(instruction_ptr->opdb_operand_type) { \
	case OPERAND_CONST: \
		opdb_addr = (int*)&opdb_value; \
		LONG_LONG_VALUE(opdb_addr) = LONG_LONG_VALUE(&instruction_ptr->opdb_addr); \
		break; \
	case OPERAND_L_VARITY: \
		opdb_addr = (int*)(sp + instruction_ptr->opdb_addr); \
		break; \
	case OPERAND_T_VARITY: \
		opdb_addr = (int*)(t_varity_sp - instruction_ptr->opdb_addr - 8); \
		break; \
	case OPERAND_LINK_VARITY: \
		opdb_addr = (int*)PTR_VALUE(t_varity_sp - instruction_ptr->opdb_addr - 8); \
		break; \
	case OPERAND_FUNCTION: \
		opdb_addr = (int*)&instruction_ptr->opdb_addr; \
		break; \
	default: \
		opdb_addr = (int*)instruction_ptr->opdb_addr; \
		break; \
	}

#define GET_RET_ADDR_NOCHECK() \
	int *ret_addr;	 \
	switch(instruction_ptr->ret_operand_type) { \
	case OPERAND_L_VARITY: \
		ret_addr = (int*)(instruction_ptr->ret_addr + sp); \
		break; \
	case OPERAND_T_VARITY: \
		ret_addr = (int*)(t_varity_sp - instruction_ptr->ret_addr - 8); \
		break; \
	case OPERAND_LINK_VARITY: \
		ret_addr = (int*)(t_varity_sp - instruction_ptr->ret_addr - 8); \
		break; \
	default: \
		ret_addr = (int*)instruction_ptr->ret_addr; \
		break; \
	}

#if HARD_FAULT_CHECK
#define GET_OPDA_ADDR() \
	GET_OPDA_ADDR_NOCHECK(); \
	if(hard_fault_check((int)opda_addr)) { \
		error("Data abort @ 0x%x.\n", opda_addr); \
		return ERROR_HARD_FAULT; \
	}
#define GET_OPDB_ADDR() \
	GET_OPDB_ADDR_NOCHECK(); \
	if(hard_fault_check((int)opdb_addr)) { \
		error("Data abort @ 0x%x.\n", opdb_addr); \
		return ERROR_HARD_FAULT; \
	}
#define GET_RET_ADDR() \
	GET_RET_ADDR_NOCHECK(); \
	if(hard_fault_check((int)ret_addr)) { \
		error("Data abort @ 0x%x.\n", ret_addr); \
		return ERROR_HARD_FAULT; \
	}
#else
#define GET_OPDA_ADDR() GET_OPDA_ADDR_NOCHECK()
#define GET_OPDB_ADDR() GET_OPDB_ADDR_NOCHECK()
#define GET_RET_ADDR() GET_RET_ADDR_NOCHECK()
#endif

#define BINARY_OPT_EXEC(x)	\
	switch(mid_type) { \
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
	switch(mid_type) { \
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
	switch(mid_type) { \
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
	switch(mid_type) { \
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

#define ASSIGN_OPT_INT_EXEC(x, converted_type, converting_type, converted_ptr, converting_ptr) \
	if(converted_type == converting_type) { \
		switch(converted_type) { \
		case INT: \
		case U_INT: \
			INT_VALUE(converted_ptr) x INT_VALUE(converting_ptr); \
			break; \
		case LONG: \
		case U_LONG: \
			LONG_VALUE(converted_ptr) x LONG_VALUE(converting_ptr); \
			break; \
		case LONG_LONG: \
		case U_LONG_LONG : \
			LONG_LONG_VALUE(converted_ptr) x LONG_LONG_VALUE(converting_ptr); \
			break; \
		case SHORT: \
		case U_SHORT: \
			SHORT_VALUE(converted_ptr) x SHORT_VALUE(converting_ptr); \
			break; \
		case CHAR: \
		case U_CHAR: \
			CHAR_VALUE(converted_ptr) x U_CHAR_VALUE(converting_ptr); \
			break; \
		} \
	} else if(converted_type > converting_type) { \
		switch(converted_type) { \
		case INT: \
		case U_INT: \
		case LONG: \
		case U_LONG: \
			if(converting_type == INT || converting_type == U_INT || converting_type == LONG || converting_type == U_LONG || converting_type == LONG_LONG || converting_type == U_LONG_LONG) \
				INT_VALUE(converted_ptr) x INT_VALUE(converting_ptr); \
			else if(converting_type == SHORT || converting_type == U_SHORT) \
				INT_VALUE(converted_ptr) x SHORT_VALUE(converting_ptr); \
			else if(converting_type == CHAR || converting_type == U_CHAR) \
				INT_VALUE(converted_ptr) x CHAR_VALUE(converting_ptr); \
			break; \
		case U_SHORT: \
		case SHORT: \
			if(converting_type == U_SHORT || converting_type == SHORT) \
				SHORT_VALUE(converted_ptr) x SHORT_VALUE(converting_ptr); \
			else if(converting_type == CHAR || converting_type == U_CHAR) \
				SHORT_VALUE(converted_ptr) x CHAR_VALUE(converting_ptr); \
			break; \
		case U_CHAR: \
			CHAR_VALUE(converted_ptr) x CHAR_VALUE(converting_ptr); \
			break; \
		} \
	} else if(converted_type < converting_type) { \
		switch(converted_type) { \
		case INT: \
		case U_INT: \
			if(converting_type == U_LONG_LONG || converting_type == LONG_LONG) { \
				INT_VALUE(converted_ptr) x LONG_LONG_VALUE(converting_ptr); \
			} \
			break; \
		case U_SHORT: \
		case SHORT: \
			SHORT_VALUE(converted_ptr) x SHORT_VALUE(converting_ptr); \
			break; \
		case U_CHAR: \
		case CHAR: \
			CHAR_VALUE(converted_ptr) x CHAR_VALUE(converting_ptr); \
			break; \
		} \
	}

#define ASSIGN_OPT_EXEC(x, converted_type, converting_type, converted_ptr, converting_ptr) \
	if(converted_type == converting_type) { \
		switch(converted_type) { \
		case INT: \
		case U_INT: \
			INT_VALUE(converted_ptr) x INT_VALUE(converting_ptr); \
			break; \
		case LONG: \
		case U_LONG: \
			LONG_VALUE(converted_ptr) x LONG_VALUE(converting_ptr); \
			break; \
		case LONG_LONG: \
		case U_LONG_LONG : \
			LONG_LONG_VALUE(converted_ptr) x LONG_LONG_VALUE(converting_ptr); \
			break; \
		case SHORT: \
		case U_SHORT: \
			SHORT_VALUE(converted_ptr) x SHORT_VALUE(converting_ptr); \
			break; \
		case CHAR: \
		case U_CHAR: \
			CHAR_VALUE(converted_ptr) x U_CHAR_VALUE(converting_ptr); \
			break; \
		case FLOAT: \
			FLOAT_VALUE(converted_ptr) x FLOAT_VALUE(converting_ptr); \
			break; \
		case DOUBLE: \
			DOUBLE_VALUE(converted_ptr) x DOUBLE_VALUE(converting_ptr); \
			break; \
		} \
	} else if(converted_type > converting_type) { \
		switch(converted_type) { \
		case INT: \
		case U_INT: \
		case LONG: \
		case U_LONG: \
			if(converting_type == INT || converting_type == U_INT || converting_type == LONG || converting_type == U_LONG) \
				INT_VALUE(converted_ptr) x INT_VALUE(converting_ptr); \
			else if(converting_type == SHORT || converting_type == U_SHORT) \
				INT_VALUE(converted_ptr) x SHORT_VALUE(converting_ptr); \
			else if(converting_type == CHAR || converting_type == U_CHAR) \
				INT_VALUE(converted_ptr) x CHAR_VALUE(converting_ptr); \
			break; \
		case U_SHORT: \
		case SHORT: \
			if(converting_type == U_SHORT || converting_type == SHORT) \
				SHORT_VALUE(converted_ptr) x SHORT_VALUE(converting_ptr); \
			else if(converting_type == CHAR || converting_type == U_CHAR) \
				SHORT_VALUE(converted_ptr) x CHAR_VALUE(converting_ptr); \
			break; \
		case U_CHAR: \
			CHAR_VALUE(converted_ptr) x CHAR_VALUE(converting_ptr); \
			break; \
		case DOUBLE: \
			switch(converting_type) { \
			case U_CHAR: \
				DOUBLE_VALUE(converted_ptr) x U_CHAR_VALUE(converting_ptr); \
				break; \
			case CHAR: \
				DOUBLE_VALUE(converted_ptr) x CHAR_VALUE(converting_ptr); \
				break; \
			case U_SHORT: \
				DOUBLE_VALUE(converted_ptr) x U_SHORT_VALUE(converting_ptr); \
				break; \
			case SHORT: \
				DOUBLE_VALUE(converted_ptr) x SHORT_VALUE(converting_ptr); \
				break; \
			case U_INT: \
				DOUBLE_VALUE(converted_ptr) x (double)U_INT_VALUE(converting_ptr); \
				break; \
			case INT: \
				DOUBLE_VALUE(converted_ptr) x INT_VALUE(converting_ptr); \
				break; \
			case U_LONG: \
				DOUBLE_VALUE(converted_ptr) x (double)U_LONG_VALUE(converting_ptr); \
				break; \
			case LONG: \
				DOUBLE_VALUE(converted_ptr) x LONG_VALUE(converting_ptr); \
				break; \
			case FLOAT: \
				DOUBLE_VALUE(converted_ptr) x FLOAT_VALUE(converting_ptr); \
				break; \
			case U_LONG_LONG: \
				DOUBLE_VALUE(converted_ptr) x (double)U_LONG_LONG_VALUE(converting_ptr); \
				break; \
			case LONG_LONG: \
				DOUBLE_VALUE(converted_ptr) x (double)LONG_LONG_VALUE(converting_ptr); \
				break; \
			} \
			break; \
		case FLOAT: \
			switch(converting_type) { \
			case U_CHAR: \
				FLOAT_VALUE(converted_ptr) x U_CHAR_VALUE(converting_ptr); \
				break; \
			case CHAR: \
				FLOAT_VALUE(converted_ptr) x CHAR_VALUE(converting_ptr); \
				break; \
			case U_SHORT: \
				FLOAT_VALUE(converted_ptr) x U_SHORT_VALUE(converting_ptr); \
				break; \
			case SHORT: \
				FLOAT_VALUE(converted_ptr) x SHORT_VALUE(converting_ptr); \
				break; \
			case U_INT: \
				FLOAT_VALUE(converted_ptr) x (float)U_INT_VALUE(converting_ptr); \
				break; \
			case INT: \
				FLOAT_VALUE(converted_ptr) x (float)INT_VALUE(converting_ptr); \
				break; \
			case U_LONG: \
				FLOAT_VALUE(converted_ptr) x (float)U_LONG_VALUE(converting_ptr); \
				break; \
			case LONG: \
				FLOAT_VALUE(converted_ptr) x (float)LONG_VALUE(converting_ptr); \
				break; \
			case U_LONG_LONG: \
				FLOAT_VALUE(converted_ptr) x (float)U_LONG_LONG_VALUE(converting_ptr); \
				break; \
			case LONG_LONG: \
				FLOAT_VALUE(converted_ptr) x (float)LONG_LONG_VALUE(converting_ptr); \
				break; \
			} \
			break; \
		} \
	} else if(converted_type < converting_type) { \
		switch(converted_type) { \
		case FLOAT: \
			FLOAT_VALUE(converted_ptr) x (float)DOUBLE_VALUE(converting_ptr); \
			break; \
		case INT: \
		case U_INT: \
			if(converting_type == DOUBLE) { \
				INT_VALUE(converted_ptr) x (int)DOUBLE_VALUE(converting_ptr); \
			} else if(converting_type == FLOAT) { \
				INT_VALUE(converted_ptr) x (int)FLOAT_VALUE(converting_ptr); \
			} else if(converting_type == U_LONG_LONG || converting_type == LONG_LONG) { \
				INT_VALUE(converted_ptr) x (int)LONG_LONG_VALUE(converting_ptr); \
			} \
			break; \
		case U_SHORT: \
		case SHORT: \
			if(converting_type == DOUBLE) { \
				SHORT_VALUE(converted_ptr) x (short)DOUBLE_VALUE(converting_ptr); \
			} else if(converting_type == FLOAT) { \
				SHORT_VALUE(converted_ptr) x (short)FLOAT_VALUE(converting_ptr); \
			} else { \
				SHORT_VALUE(converted_ptr) x SHORT_VALUE(converting_ptr); \
			} \
			break; \
		case U_CHAR: \
		case CHAR: \
			if(converting_type == DOUBLE) { \
				CHAR_VALUE(converted_ptr) x (char)DOUBLE_VALUE(converting_ptr); \
			} else if(converting_type == FLOAT) { \
				CHAR_VALUE(converted_ptr) x (char)FLOAT_VALUE(converting_ptr); \
			} else { \
				CHAR_VALUE(converted_ptr) x CHAR_VALUE(converting_ptr); \
			} \
			break; \
		} \
	}

typedef int (*opt_handle_func)(c_interpreter*);
opt_handle_func opt_handle[OPERATOR_TYPE_NUM];

int get_ret_type(int a, int b)//TODO:内联
{
	if(a <= VOID && b <= VOID) {
		return a < b ? b : a;
	} else if(a == STRUCT || b == STRUCT) {
		return ERROR_ILLEGAL_OPERAND;
	} else if(a >= PTR && b < VOID || a < VOID && b >= PTR) {
		return PTR;
	} else if(a >= PTR && b >= PTR) {
		return INT;
	}
	return INT;
}

int varity_convert(void *converted_ptr, int converted_type, void *converting_ptr, int converting_type)
{
	if(converted_type == converting_type) {
		kmemcpy(converted_ptr, converting_ptr, get_varity_size(converting_type, 0, 0));
	} else if(converted_type > converting_type) {
		switch(converted_type) {
		case INT:
		case U_INT:
		case LONG:
		case U_LONG:
			if(converting_type == INT || converting_type == U_INT || converting_type == LONG || converting_type == U_LONG || converting_type == LONG_LONG || converting_type == U_LONG_LONG)
				INT_VALUE(converted_ptr) = INT_VALUE(converting_ptr);
			else if(converting_type == SHORT || converting_type == U_SHORT)
				INT_VALUE(converted_ptr) = SHORT_VALUE(converting_ptr);
			else if(converting_type == CHAR || converting_type == U_CHAR)
				INT_VALUE(converted_ptr) = CHAR_VALUE(converting_ptr);
			break;
		case U_SHORT:
		case SHORT:
			if(converting_type == U_SHORT || converting_type == SHORT)
				SHORT_VALUE(converted_ptr) = SHORT_VALUE(converting_ptr);
			else if(converting_type == CHAR || converting_type == U_CHAR)
				SHORT_VALUE(converted_ptr) = CHAR_VALUE(converting_ptr);
			break;
		case U_CHAR:
			CHAR_VALUE(converted_ptr) = CHAR_VALUE(converting_ptr);
			break;
		case DOUBLE:
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
				DOUBLE_VALUE(converted_ptr) = (double)U_LONG_LONG_VALUE(converting_ptr);
				break;
			case LONG_LONG:
				DOUBLE_VALUE(converted_ptr) = (double)LONG_LONG_VALUE(converting_ptr);
				break;
			}
			break;
		case FLOAT:
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
				FLOAT_VALUE(converted_ptr) = (float)U_INT_VALUE(converting_ptr);
				break;
			case INT:
				FLOAT_VALUE(converted_ptr) = (float)INT_VALUE(converting_ptr);
				break;
			case U_LONG:
				FLOAT_VALUE(converted_ptr) = (float)U_LONG_VALUE(converting_ptr);
				break;
			case LONG:
				FLOAT_VALUE(converted_ptr) = (float)LONG_VALUE(converting_ptr);
				break;
			case U_LONG_LONG:
				FLOAT_VALUE(converted_ptr) = (float)U_LONG_LONG_VALUE(converting_ptr);
				break;
			case LONG_LONG:
				FLOAT_VALUE(converted_ptr) = (float)LONG_LONG_VALUE(converting_ptr);
				break;
			}
			break;
		case PTR:
			if(converting_type == INT || converting_type == U_INT || converting_type == LONG || converting_type == U_LONG) {
				PTR_VALUE(converted_ptr) = (void*)INT_VALUE(converting_ptr);
			} else if(converting_type == CHAR || converting_type == U_CHAR) {
				PTR_VALUE(converted_ptr) = (void*)(PLATFORM_WORD)CHAR_VALUE(converting_ptr);
			} else if(converting_type == SHORT || converting_type == U_SHORT) {
				PTR_VALUE(converted_ptr) = (void*)(PLATFORM_WORD)SHORT_VALUE(converting_ptr);
			} else if(converting_type > CHAR) {
				PTR_VALUE(converted_ptr) = (void*)INT_VALUE(converting_ptr);
			}
			break;
		}
	} else if(converted_type < converting_type) {
		switch(converted_type) {
		case FLOAT:
			FLOAT_VALUE(converted_ptr) = (float)DOUBLE_VALUE(converting_ptr);
			break;
		case INT:
		case U_INT:
			if(converting_type == DOUBLE) {
				INT_VALUE(converted_ptr) = (int)DOUBLE_VALUE(converting_ptr);
			} else if(converting_type == FLOAT) {
				INT_VALUE(converted_ptr) = (int)FLOAT_VALUE(converting_ptr);
			} else if(converting_type == U_LONG_LONG || converting_type == LONG_LONG) {
				INT_VALUE(converted_ptr) = (int)LONG_LONG_VALUE(converting_ptr);
			}
			break;
		case U_SHORT:
		case SHORT:
			if(converting_type == DOUBLE) {
				SHORT_VALUE(converted_ptr) = (short)DOUBLE_VALUE(converting_ptr);
			} else if(converting_type == FLOAT) {
				SHORT_VALUE(converted_ptr) = (short)FLOAT_VALUE(converting_ptr);
			} else {
				SHORT_VALUE(converted_ptr) = SHORT_VALUE(converting_ptr);
			}
			break;
		case U_CHAR:
		case CHAR:
			if(converting_type == DOUBLE) {
				CHAR_VALUE(converted_ptr) = (char)DOUBLE_VALUE(converting_ptr);
			} else if(converting_type == FLOAT) {
				CHAR_VALUE(converted_ptr) = (char)FLOAT_VALUE(converting_ptr);
			} else {
				CHAR_VALUE(converted_ptr) = CHAR_VALUE(converting_ptr);
			}
			break;
		}
	}
	return ERROR_NO;
}

int exec_invisible_type_convert(c_interpreter* interpreter_ptr, int*& opda_addr, int*& opdb_addr)
{
	register int ret_type;
	mid_code* instruction_ptr = interpreter_ptr->pc;
	if(instruction_ptr->opda_varity_type < VOID && instruction_ptr->opdb_varity_type < VOID) {
		ret_type = instruction_ptr->opda_varity_type < instruction_ptr->opdb_varity_type ? instruction_ptr->opdb_varity_type : instruction_ptr->opda_varity_type;
		if(ret_type < INT)
			ret_type = INT;
	} else if(instruction_ptr->opda_varity_type >= PTR && instruction_ptr->opdb_varity_type < VOID || instruction_ptr->opdb_varity_type >= PTR && instruction_ptr->opda_varity_type < VOID) {
		ret_type = PTR;
	} else if(instruction_ptr->opda_varity_type >= PTR && instruction_ptr->opdb_varity_type >= PTR) {
		ret_type = INT;
	} else
		ret_type = INT;
	if(instruction_ptr->opda_varity_type != ret_type && instruction_ptr->opda_varity_type < PTR) {
		varity_convert(&interpreter_ptr->mid_ret.opda, ret_type, (void*)opda_addr, instruction_ptr->opda_varity_type);
		opda_addr = (int*)&interpreter_ptr->mid_ret.opda;
	}
	if (instruction_ptr->opdb_varity_type != ret_type && instruction_ptr->opdb_varity_type < PTR) {
		varity_convert(&interpreter_ptr->mid_ret.opdb, ret_type, (void*)opdb_addr, instruction_ptr->opdb_varity_type);
		opdb_addr = (int*)&interpreter_ptr->mid_ret.opdb;
	}
	return ret_type;
}

int exec_opt_preprocess(mid_code *instruction_ptr, int *&opda_addr, int *&opdb_addr)
{
	int converting_varity_type, ret_type;
	double converted_varity;
	void *converted_varity_ptr, *converting_varity_ptr;
	ret_type = get_ret_type(instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type);
	if(instruction_ptr->opda_varity_type != instruction_ptr->opdb_varity_type && instruction_ptr->opda_varity_type < PTR) {
		converted_varity_ptr = &converted_varity;
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

int c_interpreter::opt_asl_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	BINARY_ARG_INT_OPT_EXEC(<<);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, mid_type, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_asr_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	BINARY_ARG_INT_OPT_EXEC(>>);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, mid_type, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_big_equ_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	BINARY_RET_INT_OPT_EXEC(>=);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, mid_type, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_small_equ_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	BINARY_RET_INT_OPT_EXEC(<=);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, mid_type, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_equ_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	BINARY_RET_INT_OPT_EXEC(==);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, INT, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_not_equ_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	BINARY_RET_INT_OPT_EXEC(!=);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, INT, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_devide_assign_handle(c_interpreter *interpreter_ptr)
{
	mid_code *instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	ASSIGN_OPT_EXEC(/=, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, opda_addr, opdb_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)opda_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->opda_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_mul_assign_handle(c_interpreter *interpreter_ptr)
{
	mid_code *instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	ASSIGN_OPT_EXEC(*=, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, opda_addr, opdb_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)opda_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->opda_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_mod_assign_handle(c_interpreter *interpreter_ptr)
{
	mid_code *instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	ASSIGN_OPT_INT_EXEC(%=, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, opda_addr, opdb_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)opda_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->opda_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_add_assign_handle(c_interpreter *interpreter_ptr)
{
	mid_code *instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	register int converted_type = instruction_ptr->opda_varity_type, converting_type = instruction_ptr->opdb_varity_type;
	register void *converted_ptr = (void*)opda_addr, *converting_ptr = (void*)opdb_addr;
	if(converted_type == converting_type) { 
		switch(converted_type) { 
		case INT: 
		case U_INT: 
			INT_VALUE(converted_ptr) += INT_VALUE(converting_ptr); 
			break; 
		case LONG: 
		case U_LONG: 
			LONG_VALUE(converted_ptr) += LONG_VALUE(converting_ptr); 
			break; 
		case LONG_LONG: 
		case U_LONG_LONG : 
			LONG_LONG_VALUE(converted_ptr) += LONG_LONG_VALUE(converting_ptr); 
			break; 
		case SHORT:
		case U_SHORT:
			SHORT_VALUE(converted_ptr) += SHORT_VALUE(converting_ptr);
			break;
		case CHAR:
		case U_CHAR:
			CHAR_VALUE(converted_ptr) += U_CHAR_VALUE(converting_ptr);
			break;
		case FLOAT:
			FLOAT_VALUE(converted_ptr) += FLOAT_VALUE(converting_ptr);
			break;
		case DOUBLE:
			DOUBLE_VALUE(converted_ptr) += DOUBLE_VALUE(converting_ptr);
			break;
		}
	} else if(converted_type > converting_type) {
		switch(converted_type) {
		case INT:
		case U_INT:
		case LONG:
		case U_LONG:
			if(converting_type == INT || converting_type == U_INT || converting_type == LONG || converting_type == U_LONG || converting_type == LONG_LONG || converting_type == U_LONG_LONG)
				INT_VALUE(converted_ptr) += INT_VALUE(converting_ptr);
			else if(converting_type == SHORT || converting_type == U_SHORT)
				INT_VALUE(converted_ptr) += SHORT_VALUE(converting_ptr);
			else if(converting_type == CHAR || converting_type == U_CHAR)
				INT_VALUE(converted_ptr) += CHAR_VALUE(converting_ptr);
			break;
		case U_SHORT:
		case SHORT:
			if(converting_type == U_SHORT || converting_type == SHORT)
				SHORT_VALUE(converted_ptr) += SHORT_VALUE(converting_ptr);
			else if(converting_type == CHAR || converting_type == U_CHAR)
				SHORT_VALUE(converted_ptr) += CHAR_VALUE(converting_ptr);
			break;
		case U_CHAR:
			CHAR_VALUE(converted_ptr) += CHAR_VALUE(converting_ptr);
			break;
		case DOUBLE:
			switch(converting_type) {
			case U_CHAR:
				DOUBLE_VALUE(converted_ptr) += U_CHAR_VALUE(converting_ptr);
				break;
			case CHAR:
				DOUBLE_VALUE(converted_ptr) += CHAR_VALUE(converting_ptr);
				break;
			case U_SHORT:
				DOUBLE_VALUE(converted_ptr) += U_SHORT_VALUE(converting_ptr);
				break;
			case SHORT:
				DOUBLE_VALUE(converted_ptr) += SHORT_VALUE(converting_ptr);
				break;
			case U_INT:
				DOUBLE_VALUE(converted_ptr) += U_INT_VALUE(converting_ptr);
				break;
			case INT:
				DOUBLE_VALUE(converted_ptr) += INT_VALUE(converting_ptr);
				break;
			case U_LONG:
				DOUBLE_VALUE(converted_ptr) += U_LONG_VALUE(converting_ptr);
				break;
			case LONG:
				DOUBLE_VALUE(converted_ptr) += LONG_VALUE(converting_ptr);
				break;
			case FLOAT:
				DOUBLE_VALUE(converted_ptr) += FLOAT_VALUE(converting_ptr);
				break;
			case U_LONG_LONG:
				DOUBLE_VALUE(converted_ptr) += U_LONG_LONG_VALUE(converting_ptr);
				break;
			case LONG_LONG:
				DOUBLE_VALUE(converted_ptr) += LONG_LONG_VALUE(converting_ptr);
				break;
			}
			break;
		case FLOAT:
			switch(converting_type) {
			case U_CHAR:
				FLOAT_VALUE(converted_ptr) += U_CHAR_VALUE(converting_ptr);
				break;
			case CHAR:
				FLOAT_VALUE(converted_ptr) += CHAR_VALUE(converting_ptr);
				break;
			case U_SHORT:
				FLOAT_VALUE(converted_ptr) += U_SHORT_VALUE(converting_ptr);
				break;
			case SHORT:
				FLOAT_VALUE(converted_ptr) += SHORT_VALUE(converting_ptr);
				break;
			case U_INT:
				FLOAT_VALUE(converted_ptr) += U_INT_VALUE(converting_ptr);
				break;
			case INT:
				FLOAT_VALUE(converted_ptr) += INT_VALUE(converting_ptr);
				break;
			case U_LONG:
				FLOAT_VALUE(converted_ptr) += U_LONG_VALUE(converting_ptr);
				break;
			case LONG:
				FLOAT_VALUE(converted_ptr) += LONG_VALUE(converting_ptr);
				break;
			case U_LONG_LONG:
				FLOAT_VALUE(converted_ptr) += U_LONG_LONG_VALUE(converting_ptr);
				break;
			case LONG_LONG:
				FLOAT_VALUE(converted_ptr) += LONG_LONG_VALUE(converting_ptr);
				break;
			}
			break;
		case PTR:
			if(converting_type == INT || converting_type == U_INT || converting_type == LONG || converting_type == U_LONG) {
				PTR_N_VALUE(converted_ptr) += INT_VALUE(converting_ptr) * instruction_ptr->data;
			} else if(converting_type == CHAR || converting_type == U_CHAR) {
				PTR_N_VALUE(converted_ptr) += CHAR_VALUE(converting_ptr) * instruction_ptr->data;
			} else if(converting_type == SHORT || converting_type == U_SHORT) {
				PTR_N_VALUE(converted_ptr) += SHORT_VALUE(converting_ptr) * instruction_ptr->data;
			} else if(converting_type > CHAR) {
				PTR_N_VALUE(converted_ptr) += INT_VALUE(converting_ptr) * instruction_ptr->data;
			}
			break;
		}
	} else if(converted_type < converting_type) {
		switch(converted_type) {
		case FLOAT:
			FLOAT_VALUE(converted_ptr) += (float)DOUBLE_VALUE(converting_ptr);
			break;
		case INT:
		case U_INT:
			if(converting_type == DOUBLE) {
				INT_VALUE(converted_ptr) += (int)DOUBLE_VALUE(converting_ptr);
			} else if(converting_type == FLOAT) {
				INT_VALUE(converted_ptr) += (int)FLOAT_VALUE(converting_ptr);
			} else if(converting_type == U_LONG_LONG || converting_type == LONG_LONG) {
				INT_VALUE(converted_ptr) += (int)LONG_LONG_VALUE(converting_ptr);
			}
			break;
		case U_SHORT:
		case SHORT:
			if(converting_type == DOUBLE) {
				SHORT_VALUE(converted_ptr) += (short)DOUBLE_VALUE(converting_ptr);
			} else if(converting_type == FLOAT) {
				SHORT_VALUE(converted_ptr) += (short)FLOAT_VALUE(converting_ptr);
			} else {
				SHORT_VALUE(converted_ptr) += SHORT_VALUE(converting_ptr);
			}
			break;
		case U_CHAR:
		case CHAR:
			if(converting_type == DOUBLE) {
				CHAR_VALUE(converted_ptr) += (char)DOUBLE_VALUE(converting_ptr);
			} else if(converting_type == FLOAT) {
				CHAR_VALUE(converted_ptr) += (char)FLOAT_VALUE(converting_ptr);
			} else {
				CHAR_VALUE(converted_ptr) += CHAR_VALUE(converting_ptr);
			}
			break;
		}
	}
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)opda_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->opda_varity_type;
	return 0;
}

int c_interpreter::opt_minus_assign_handle(c_interpreter *interpreter_ptr)
{
	mid_code *instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	register int converted_type = instruction_ptr->opda_varity_type, converting_type = instruction_ptr->opdb_varity_type;
	register void *converted_ptr = (void*)opda_addr, *converting_ptr = (void*)opdb_addr;
	if(converted_type == converting_type) { 
		switch(converted_type) { 
		case INT: 
		case U_INT: 
			INT_VALUE(converted_ptr) -= INT_VALUE(converting_ptr); 
			break; 
		case LONG: 
		case U_LONG: 
			LONG_VALUE(converted_ptr) -= LONG_VALUE(converting_ptr); 
			break; 
		case LONG_LONG: 
		case U_LONG_LONG : 
			LONG_LONG_VALUE(converted_ptr) -= LONG_LONG_VALUE(converting_ptr); 
			break; 
		case SHORT:
		case U_SHORT:
			SHORT_VALUE(converted_ptr) -= SHORT_VALUE(converting_ptr);
			break;
		case CHAR:
		case U_CHAR:
			CHAR_VALUE(converted_ptr) -= U_CHAR_VALUE(converting_ptr);
			break;
		case FLOAT:
			FLOAT_VALUE(converted_ptr) -= FLOAT_VALUE(converting_ptr);
			break;
		case DOUBLE:
			DOUBLE_VALUE(converted_ptr) -= DOUBLE_VALUE(converting_ptr);
			break;
		}
	} else if(converted_type > converting_type) {
		switch(converted_type) {
		case INT:
		case U_INT:
		case LONG:
		case U_LONG:
			if(converting_type == INT || converting_type == U_INT || converting_type == LONG || converting_type == U_LONG || converting_type == LONG_LONG || converting_type == U_LONG_LONG)
				INT_VALUE(converted_ptr) -= INT_VALUE(converting_ptr);
			else if(converting_type == SHORT || converting_type == U_SHORT)
				INT_VALUE(converted_ptr) -= SHORT_VALUE(converting_ptr);
			else if(converting_type == CHAR || converting_type == U_CHAR)
				INT_VALUE(converted_ptr) -= CHAR_VALUE(converting_ptr);
			break;
		case U_SHORT:
		case SHORT:
			if(converting_type == U_SHORT || converting_type == SHORT)
				SHORT_VALUE(converted_ptr) -= SHORT_VALUE(converting_ptr);
			else if(converting_type == CHAR || converting_type == U_CHAR)
				SHORT_VALUE(converted_ptr) -= CHAR_VALUE(converting_ptr);
			break;
		case U_CHAR:
			CHAR_VALUE(converted_ptr) -= CHAR_VALUE(converting_ptr);
			break;
		case DOUBLE:
			switch(converting_type) {
			case U_CHAR:
				DOUBLE_VALUE(converted_ptr) -= U_CHAR_VALUE(converting_ptr);
				break;
			case CHAR:
				DOUBLE_VALUE(converted_ptr) -= CHAR_VALUE(converting_ptr);
				break;
			case U_SHORT:
				DOUBLE_VALUE(converted_ptr) -= U_SHORT_VALUE(converting_ptr);
				break;
			case SHORT:
				DOUBLE_VALUE(converted_ptr) -= SHORT_VALUE(converting_ptr);
				break;
			case U_INT:
				DOUBLE_VALUE(converted_ptr) -= U_INT_VALUE(converting_ptr);
				break;
			case INT:
				DOUBLE_VALUE(converted_ptr) -= INT_VALUE(converting_ptr);
				break;
			case U_LONG:
				DOUBLE_VALUE(converted_ptr) -= U_LONG_VALUE(converting_ptr);
				break;
			case LONG:
				DOUBLE_VALUE(converted_ptr) -= LONG_VALUE(converting_ptr);
				break;
			case FLOAT:
				DOUBLE_VALUE(converted_ptr) -= FLOAT_VALUE(converting_ptr);
				break;
			case U_LONG_LONG:
				DOUBLE_VALUE(converted_ptr) -= U_LONG_LONG_VALUE(converting_ptr);
				break;
			case LONG_LONG:
				DOUBLE_VALUE(converted_ptr) -= LONG_LONG_VALUE(converting_ptr);
				break;
			}
			break;
		case FLOAT:
			switch(converting_type) {
			case U_CHAR:
				FLOAT_VALUE(converted_ptr) -= U_CHAR_VALUE(converting_ptr);
				break;
			case CHAR:
				FLOAT_VALUE(converted_ptr) -= CHAR_VALUE(converting_ptr);
				break;
			case U_SHORT:
				FLOAT_VALUE(converted_ptr) -= U_SHORT_VALUE(converting_ptr);
				break;
			case SHORT:
				FLOAT_VALUE(converted_ptr) -= SHORT_VALUE(converting_ptr);
				break;
			case U_INT:
				FLOAT_VALUE(converted_ptr) -= U_INT_VALUE(converting_ptr);
				break;
			case INT:
				FLOAT_VALUE(converted_ptr) -= INT_VALUE(converting_ptr);
				break;
			case U_LONG:
				FLOAT_VALUE(converted_ptr) -= U_LONG_VALUE(converting_ptr);
				break;
			case LONG:
				FLOAT_VALUE(converted_ptr) -= LONG_VALUE(converting_ptr);
				break;
			case U_LONG_LONG:
				FLOAT_VALUE(converted_ptr) -= U_LONG_LONG_VALUE(converting_ptr);
				break;
			case LONG_LONG:
				FLOAT_VALUE(converted_ptr) -= LONG_LONG_VALUE(converting_ptr);
				break;
			}
			break;
		case PTR:
			if(converting_type == INT || converting_type == U_INT || converting_type == LONG || converting_type == U_LONG) {
				PTR_N_VALUE(converted_ptr) -= INT_VALUE(converting_ptr);
			} else if(converting_type == CHAR || converting_type == U_CHAR) {
				PTR_N_VALUE(converted_ptr) -= CHAR_VALUE(converting_ptr);
			} else if(converting_type == SHORT || converting_type == U_SHORT) {
				PTR_N_VALUE(converted_ptr) -= SHORT_VALUE(converting_ptr);
			} else if(converting_type > CHAR) {
				PTR_N_VALUE(converted_ptr) -= INT_VALUE(converting_ptr);
			}
			break;
		}
	} else if(converted_type < converting_type) {
		switch(converted_type) {
		case FLOAT:
			FLOAT_VALUE(converted_ptr) -= (float)DOUBLE_VALUE(converting_ptr);
			break;
		case INT:
		case U_INT:
			if(converting_type == DOUBLE) {
				INT_VALUE(converted_ptr) -= (int)DOUBLE_VALUE(converting_ptr);
			} else if(converting_type == FLOAT) {
				INT_VALUE(converted_ptr) -= (int)FLOAT_VALUE(converting_ptr);
			} else if(converting_type == U_LONG_LONG || converting_type == LONG_LONG) {
				INT_VALUE(converted_ptr) -= (int)LONG_LONG_VALUE(converting_ptr);
			}
			break;
		case U_SHORT:
		case SHORT:
			if(converting_type == DOUBLE) {
				SHORT_VALUE(converted_ptr) -= (short)DOUBLE_VALUE(converting_ptr);
			} else if(converting_type == FLOAT) {
				SHORT_VALUE(converted_ptr) -= (short)FLOAT_VALUE(converting_ptr);
			} else {
				SHORT_VALUE(converted_ptr) -= SHORT_VALUE(converting_ptr);
			}
			break;
		case U_CHAR:
		case CHAR:
			if(converting_type == DOUBLE) {
				CHAR_VALUE(converted_ptr) -= (char)DOUBLE_VALUE(converting_ptr);
			} else if(converting_type == FLOAT) {
				CHAR_VALUE(converted_ptr) -= (char)FLOAT_VALUE(converting_ptr);
			} else {
				CHAR_VALUE(converted_ptr) -= CHAR_VALUE(converting_ptr);
			}
			break;
		}
	}
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)opda_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->opda_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_bit_and_assign_handle(c_interpreter *interpreter_ptr)
{
	mid_code *instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	ASSIGN_OPT_INT_EXEC(&=, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, opda_addr, opdb_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)opda_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->opda_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_xor_assign_handle(c_interpreter *interpreter_ptr)
{
	mid_code *instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	ASSIGN_OPT_INT_EXEC(^=, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, opda_addr, opdb_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)opda_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->opda_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_bit_or_assign_handle(c_interpreter *interpreter_ptr)
{
	mid_code *instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	ASSIGN_OPT_INT_EXEC(|=, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, opda_addr, opdb_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)opda_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->opda_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_and_handle(c_interpreter *interpreter_ptr)
{
	int ret;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	int converted_varityb;
	ret = varity_convert(&converted_varityb, INT, opdb_addr, instruction_ptr->opdb_varity_type);
	if(ret)
		return ERROR_TYPE_CONVERT;
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, INT, ret_addr, &converted_varityb);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return 0;
}

int c_interpreter::opt_or_handle(c_interpreter *interpreter_ptr)
{
	int ret;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	int converted_varityb;
	ret = varity_convert(&converted_varityb, INT, opdb_addr, instruction_ptr->opdb_varity_type);
	if(ret)
		return ERROR_TYPE_CONVERT;
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, INT, ret_addr, &converted_varityb);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_member_handle(c_interpreter *interpreter_ptr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	PTR_N_VALUE(ret_addr) = (PLATFORM_WORD)opda_addr + PTR_N_VALUE(opdb_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_minus_handle(c_interpreter *interpreter_ptr)
{
	int ret;
	int ret_type, converting_varity_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	double converted_varity, mid_ret;
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
		ret = varity_convert(converted_varity_ptr, ret_type, converting_varity_ptr, converting_varity_type);
		if(ret) return ret;
	}
	if(ret_type == U_LONG || ret_type == LONG || ret_type == U_INT || ret_type == INT) {
		if(instruction_ptr->opda_varity_type == PTR)
			INT_VALUE(&mid_ret) = (PTR_N_VALUE(opda_addr) - PTR_N_VALUE(opdb_addr)) / instruction_ptr->data;
		else
			INT_VALUE(&mid_ret) = INT_VALUE(opda_addr) - INT_VALUE(opdb_addr);
	} else if(ret_type == U_SHORT || ret_type == SHORT) {
		SHORT_VALUE(&mid_ret) = SHORT_VALUE(opda_addr) - SHORT_VALUE(opdb_addr);
	} else if(ret_type == U_CHAR || ret_type == CHAR) {
		CHAR_VALUE(&mid_ret) = CHAR_VALUE(opda_addr) - CHAR_VALUE(opdb_addr);
	} else if(ret_type == DOUBLE) {
		DOUBLE_VALUE(&mid_ret) = DOUBLE_VALUE(opda_addr) - DOUBLE_VALUE(opdb_addr);
	} else if(ret_type == FLOAT) {
		FLOAT_VALUE(&mid_ret) = FLOAT_VALUE(opda_addr) - FLOAT_VALUE(opdb_addr);
	} else if(ret_type == PTR) {
		PTR_N_VALUE(&mid_ret) = PTR_N_VALUE(opda_addr) - INT_VALUE(opdb_addr) * instruction_ptr->data;
	}
	varity_convert(ret_addr, instruction_ptr->ret_varity_type, &mid_ret, ret_type);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_bit_revert_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = INT;
	UNARY_OPT_EXEC(~);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, mid_type, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_mul_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = exec_invisible_type_convert(interpreter_ptr, opda_addr, opdb_addr);
	//mid_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	BINARY_OPT_EXEC(*);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, mid_type, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return 0;
}

int c_interpreter::opt_bit_and_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	BINARY_ARG_INT_OPT_EXEC(&);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, mid_type, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_not_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = INT;
	UNARY_OPT_EXEC(!);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, mid_type, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_divide_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
#if DIVIDE_ZERO_CHECK == 1
	if(!is_non_zero(instruction_ptr->opdb_varity_type, opdb_addr)) {
		error("Divide zero.\n");
		return ERROR_DEVIDE_ZERO;
	}
#endif
	BINARY_OPT_EXEC(/);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, mid_type, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_mod_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	BINARY_ARG_INT_OPT_EXEC(%);
	switch(mid_type) {
	case INT:
	case LONG:
		INT_VALUE(ret_addr) = INT_VALUE(opda_addr) % INT_VALUE(opdb_addr);
		break;
	case U_INT:
	case U_LONG:
		U_INT_VALUE(ret_addr) = U_INT_VALUE(opda_addr) % U_INT_VALUE(opdb_addr);
		break;
	case LONG_LONG:
		LONG_LONG_VALUE(ret_addr) = LONG_LONG_VALUE(opda_addr) % LONG_LONG_VALUE(opdb_addr);
		break;
	case U_LONG_LONG:
		U_LONG_LONG_VALUE(ret_addr) = U_LONG_LONG_VALUE(opda_addr) % U_LONG_LONG_VALUE(opdb_addr);
	}
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, mid_type, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

ITCM_TEXT int c_interpreter::opt_plus_handle(c_interpreter *interpreter_ptr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
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
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_big_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	BINARY_RET_INT_OPT_EXEC(>);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, INT, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

ITCM_TEXT int c_interpreter::opt_small_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	BINARY_RET_INT_OPT_EXEC(<);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, INT, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_bit_xor_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	BINARY_ARG_INT_OPT_EXEC(^);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, mid_type, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_bit_or_handle(c_interpreter *interpreter_ptr)
{
	int mid_type;
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	mid_type = exec_opt_preprocess(instruction_ptr, opda_addr, opdb_addr);
	BINARY_ARG_INT_OPT_EXEC(|);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, mid_type, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

ITCM_TEXT int c_interpreter::opt_assign_handle(c_interpreter *interpreter_ptr)
{
	mid_code *instruction_ptr = interpreter_ptr->pc;
	register int converted_type = instruction_ptr->opda_varity_type, converting_type = instruction_ptr->opdb_varity_type;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	double align_varity;
	int *converted_ptr;
	if(converted_type == converting_type) { 
		switch(converted_type) { 
		case INT: 
		case U_INT: 
			INT_VALUE(opda_addr) = INT_VALUE(opdb_addr); 
			break; 
		case LONG: 
		case U_LONG: 
			LONG_VALUE(opda_addr) = LONG_VALUE(opdb_addr); 
			break; 
		case LONG_LONG: 
		case U_LONG_LONG : 
			LONG_LONG_VALUE(opda_addr) = LONG_LONG_VALUE(opdb_addr); 
			break; 
		case SHORT:
		case U_SHORT:
			SHORT_VALUE(opda_addr) = SHORT_VALUE(opdb_addr);
			break;
		case CHAR:
		case U_CHAR:
			CHAR_VALUE(opda_addr) = U_CHAR_VALUE(opdb_addr);
			break;
		case FLOAT:
			FLOAT_VALUE(opda_addr) = FLOAT_VALUE(opdb_addr);
			break;
		case DOUBLE:
			DOUBLE_VALUE(opda_addr) = DOUBLE_VALUE(opdb_addr);
			break;
		case PTR:
			PTR_VALUE(opda_addr) = PTR_VALUE(opdb_addr);
		}
	} else if(converted_type > converting_type) {
		switch(converted_type) {
		case INT:
		case U_INT:
		case LONG:
		case U_LONG:
			if(converting_type == INT || converting_type == U_INT || converting_type == LONG || converting_type == U_LONG || converting_type == LONG_LONG || converting_type == U_LONG_LONG)
				INT_VALUE(opda_addr) = INT_VALUE(opdb_addr);
			else if(converting_type == SHORT || converting_type == U_SHORT)
				INT_VALUE(opda_addr) = SHORT_VALUE(opdb_addr);
			else if(converting_type == CHAR || converting_type == U_CHAR)
				INT_VALUE(opda_addr) = CHAR_VALUE(opdb_addr);
			break;
		case LONG_LONG:
		case U_LONG_LONG:
			if(converting_type == INT || converting_type == U_INT || converting_type == LONG || converting_type == U_LONG)
				LONG_LONG_VALUE(opda_addr) = INT_VALUE(opdb_addr);
			else if(converting_type == SHORT || converting_type == U_SHORT)
				LONG_LONG_VALUE(opda_addr) = SHORT_VALUE(opdb_addr);
			else if(converting_type == CHAR || converting_type == U_CHAR)
				LONG_LONG_VALUE(opda_addr) = CHAR_VALUE(opdb_addr);
			else if(converting_type == LONG_LONG || converting_type == U_LONG_LONG)
				LONG_LONG_VALUE(opda_addr) = LONG_LONG_VALUE(opdb_addr);
			break;
		case U_SHORT:
		case SHORT:
			if(converting_type == SHORT)
				SHORT_VALUE(opda_addr) = SHORT_VALUE(opdb_addr);
			else if(converting_type == CHAR || converting_type == U_CHAR)
				SHORT_VALUE(opda_addr) = CHAR_VALUE(opdb_addr);
			break;
		case U_CHAR:
			CHAR_VALUE(opda_addr) = CHAR_VALUE(opdb_addr);
			break;
		case DOUBLE:
		{
			if((PLATFORM_WORD)opda_addr & 7) {
				converted_ptr = (int*)&align_varity;
			} else {
				converted_ptr = opda_addr;
			}
			switch(converting_type) {
			case U_CHAR:
				DOUBLE_VALUE(converted_ptr) = U_CHAR_VALUE(opdb_addr);
				break;
			case CHAR:
				DOUBLE_VALUE(converted_ptr) = CHAR_VALUE(opdb_addr);
				break;
			case U_SHORT:
				DOUBLE_VALUE(converted_ptr) = U_SHORT_VALUE(opdb_addr);
				break;
			case SHORT:
				DOUBLE_VALUE(converted_ptr) = SHORT_VALUE(opdb_addr);
				break;
			case U_INT:
				DOUBLE_VALUE(converted_ptr) = U_INT_VALUE(opdb_addr);
				break;
			case INT:
				DOUBLE_VALUE(converted_ptr) = INT_VALUE(opdb_addr);
				break;
			case U_LONG:
				DOUBLE_VALUE(converted_ptr) = U_LONG_VALUE(opdb_addr);
				break;
			case LONG:
				DOUBLE_VALUE(converted_ptr) = LONG_VALUE(opdb_addr);
				break;
			case FLOAT:
				DOUBLE_VALUE(converted_ptr) = FLOAT_VALUE(opdb_addr);
				break;
			case U_LONG_LONG:
				DOUBLE_VALUE(converted_ptr) = (double)U_LONG_LONG_VALUE(opdb_addr);
				break;
			case LONG_LONG:
				DOUBLE_VALUE(converted_ptr) = (double)LONG_LONG_VALUE(opdb_addr);
				break;
			}
			if((PLATFORM_WORD)opda_addr & 7)
				kmemcpy(opda_addr, converted_ptr, 8);
			break;
		}
		case FLOAT:
			switch(converting_type) {
			case U_CHAR:
				FLOAT_VALUE(opda_addr) = U_CHAR_VALUE(opdb_addr);
				break;
			case CHAR:
				FLOAT_VALUE(opda_addr) = CHAR_VALUE(opdb_addr);
				break;
			case U_SHORT:
				FLOAT_VALUE(opda_addr) = U_SHORT_VALUE(opdb_addr);
				break;
			case SHORT:
				FLOAT_VALUE(opda_addr) = SHORT_VALUE(opdb_addr);
				break;
			case U_INT:
				FLOAT_VALUE(opda_addr) = (float)U_INT_VALUE(opdb_addr);
				break;
			case INT:
				FLOAT_VALUE(opda_addr) = (float)INT_VALUE(opdb_addr);
				break;
			case U_LONG:
				FLOAT_VALUE(opda_addr) = (float)U_LONG_VALUE(opdb_addr);
				break;
			case LONG:
				FLOAT_VALUE(opda_addr) = (float)LONG_VALUE(opdb_addr);
				break;
			case U_LONG_LONG:
				FLOAT_VALUE(opda_addr) = (float)U_LONG_LONG_VALUE(opdb_addr);
				break;
			case LONG_LONG:
				FLOAT_VALUE(opda_addr) = (float)LONG_LONG_VALUE(opdb_addr);
				break;
			}
			break;
		case PTR:
			if(converting_type == INT || converting_type == U_INT || converting_type == LONG || converting_type == U_LONG) {
				PTR_N_VALUE(opda_addr) = INT_VALUE(opdb_addr);
			} else if(converting_type == CHAR || converting_type == U_CHAR) {
				PTR_N_VALUE(opda_addr) = CHAR_VALUE(opdb_addr);
			} else if(converting_type == SHORT || converting_type == U_SHORT) {
				PTR_N_VALUE(opda_addr) = SHORT_VALUE(opdb_addr);
			} else if(converting_type == LONG_LONG || converting_type ==  U_LONG_LONG) {
				PTR_N_VALUE(opda_addr) = (PLATFORM_WORD)LONG_LONG_VALUE(opdb_addr);
			}
			break;
		}
	} else if(converted_type < converting_type) {
		switch(converted_type) {
		case FLOAT:
			FLOAT_VALUE(opda_addr) = (float)DOUBLE_VALUE(opdb_addr);
			break;
		case INT:
		case U_INT:
		case LONG:
		case U_LONG:
			if(converting_type == DOUBLE) {
				INT_VALUE(opda_addr) = (int)DOUBLE_VALUE(opdb_addr);
			} else if(converting_type == FLOAT) {
				INT_VALUE(opda_addr) = (int)FLOAT_VALUE(opdb_addr);
			} else if(converting_type == U_LONG_LONG || converting_type == LONG_LONG) {
				INT_VALUE(opda_addr) = (int)LONG_LONG_VALUE(opdb_addr);
			} else if(converting_type == ARRAY) {
				INT_VALUE(opda_addr) = (int)opdb_addr;
			} else if(converting_type == PTR) {
				INT_VALUE(opda_addr) = (int)PTR_VALUE(opdb_addr);
			} else if(converting_type == U_INT) {
				INT_VALUE(opda_addr) = INT_VALUE(opdb_addr);
			}
			break;
		case U_SHORT:
		case SHORT:
			if(converting_type == DOUBLE) {
				SHORT_VALUE(opda_addr) = (short)DOUBLE_VALUE(opdb_addr);
			} else if(converting_type == FLOAT) {
				SHORT_VALUE(opda_addr) = (short)FLOAT_VALUE(opdb_addr);
			} else {
				SHORT_VALUE(opda_addr) = SHORT_VALUE(opdb_addr);
			}
			break;
		case U_CHAR:
		case CHAR:
			if(converting_type == DOUBLE) {
				CHAR_VALUE(opda_addr) = (char)DOUBLE_VALUE(opdb_addr);
			} else if(converting_type == FLOAT) {
				CHAR_VALUE(opda_addr) = (char)FLOAT_VALUE(opdb_addr);
			} else {
				CHAR_VALUE(opda_addr) = CHAR_VALUE(opdb_addr);
			}
			break;
		case LONG_LONG:
		case U_LONG_LONG:
			if(converting_type == DOUBLE) {
				LONG_LONG_VALUE(opda_addr) = (long long)DOUBLE_VALUE(opdb_addr);
			} else if(converting_type == FLOAT) {
				LONG_LONG_VALUE(opda_addr) = (long long)FLOAT_VALUE(opdb_addr);
			} else if(converting_type == ARRAY) {
				LONG_LONG_VALUE(opda_addr) = (long long)opdb_addr;
			} else if(converting_type == PTR) {
				LONG_LONG_VALUE(opda_addr) = (long long)PTR_VALUE(opdb_addr);
			} else if(converting_type == U_LONG_LONG) {
				LONG_LONG_VALUE(opda_addr) = LONG_LONG_VALUE(opdb_addr);
			}
			break;
		case PTR:
			PTR_N_VALUE(opda_addr) = (PLATFORM_WORD)opdb_addr;
		}
	}
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)opda_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->opda_varity_type;
	return 0;
}

int c_interpreter::opt_negative_handle(c_interpreter *interpreter_ptr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	int mid_type = instruction_ptr->opdb_varity_type;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	switch(mid_type) {
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
		break;
	case DOUBLE:
		DOUBLE_VALUE(ret_addr) = - DOUBLE_VALUE(opdb_addr);
		break;
	case FLOAT:
		FLOAT_VALUE(ret_addr) = - FLOAT_VALUE(opdb_addr);
	}
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, mid_type, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_positive_handle(c_interpreter *interpreter_ptr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	int mid_type = instruction_ptr->opdb_varity_type;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	DOUBLE_VALUE(ret_addr) = DOUBLE_VALUE(opdb_addr);
	ASSIGN_OPT_EXEC(=, instruction_ptr->ret_varity_type, mid_type, ret_addr, ret_addr);
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_ptr_content_handle(c_interpreter *interpreter_ptr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	if(instruction_ptr->opdb_varity_type == ARRAY)
		PTR_VALUE(ret_addr) = opdb_addr;//生成链接变量
	else if(instruction_ptr->opdb_varity_type == PTR) {
		PTR_VALUE(ret_addr) = PTR_VALUE(opdb_addr);
	}
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_address_of_handle(c_interpreter *interpreter_ptr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	PTR_VALUE(ret_addr) = opdb_addr;//生成中间变量
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

int c_interpreter::opt_index_handle(c_interpreter *interpreter_ptr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDA_ADDR();
	GET_OPDB_ADDR();
	GET_RET_ADDR();
#if ARRAY_BOUND_CHECK
	int element_count = (instruction_ptr->data & (0xFF << 24)) >> 8 | instruction_ptr->opdb_varity_type << 8 | instruction_ptr->ret_varity_type;//TODO:长度128可出错
	if(INT_VALUE(opdb_addr) >= element_count) {
		error("Array bound.\n");
		return ERROR_ARRAY_BOUND;
	}
#endif
	if(instruction_ptr->opda_varity_type == ARRAY)
		INT_VALUE(ret_addr) = (int)opda_addr + INT_VALUE(opdb_addr) * (instruction_ptr->data & ~(0xFF << 24));
	else if(instruction_ptr->opda_varity_type == PTR)
		INT_VALUE(ret_addr) = INT_VALUE(opda_addr) + INT_VALUE(opdb_addr) * (instruction_ptr->data & ~(0xFF << 24));
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return ERROR_NO;
}

ITCM_TEXT int c_interpreter::opt_call_func_handle(c_interpreter *interpreter_ptr)
{//opda：入口地址 opdb：临时变量堆栈推进值 data：调用时所用栈帧大小，用于堆栈回溯 opda_varity_type：参数个数
	mid_code *instruction_ptr = interpreter_ptr->pc;
	function_info *function_ptr = (function_info*)instruction_ptr->opda_addr;
	if(!function_ptr->compile_func_flag) {
		PTR_N_VALUE(interpreter_ptr->stack_pointer - PLATFORM_WORD_LEN) = (PLATFORM_WORD)interpreter_ptr->pc;
		interpreter_ptr->tmp_varity_stack_pointer -= (int)instruction_ptr->opdb_addr;
		interpreter_ptr->pc = (mid_code*)function_ptr->mid_code_stack.get_base_addr() - 1;
#if STACK_OVERFLOW_CHECK
		if(interpreter_ptr->stack_pointer + function_ptr->stack_frame_size >= interpreter_ptr->tmp_varity_stack_pointer) {
			error("Stack overflow.\n");
			return ERROR_STACK_OVERFLOW;
		}
#endif
		return ERROR_NO;
	}
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_RET_ADDR();
	long long ret;
	PLATFORM_WORD *arg_ptr;
	arg_ptr = (PLATFORM_WORD*)interpreter_ptr->stack_pointer;
	switch(instruction_ptr->opda_varity_type) {
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
		break;
	case 4:
		func4_ptr = (func4)function_ptr->func_addr;
		ret = func4_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2), *(arg_ptr + 3));
		break;
	case 5:
		func5_ptr = (func5)function_ptr->func_addr;
		ret = func5_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2), *(arg_ptr + 3), *(arg_ptr + 4));
		break;
	case 6:
		func6_ptr = (func6)function_ptr->func_addr;
		ret = func6_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2), *(arg_ptr + 3), *(arg_ptr + 4), *(arg_ptr + 5));
		break;
	case 7:
		func7_ptr = (func7)function_ptr->func_addr;
		ret = func7_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2), *(arg_ptr + 3), *(arg_ptr + 4), *(arg_ptr + 5), *(arg_ptr + 6));
		break;
	case 8:
		func8_ptr = (func8)function_ptr->func_addr;
		ret = func8_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2), *(arg_ptr + 3), *(arg_ptr + 4), *(arg_ptr + 5), *(arg_ptr + 6), *(arg_ptr + 7));
		break;
	case 9:
		func9_ptr = (func9)function_ptr->func_addr;
		ret = func9_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2), *(arg_ptr + 3), *(arg_ptr + 4), *(arg_ptr + 5), *(arg_ptr + 6), *(arg_ptr + 7), *(arg_ptr + 8));
		break;
	case 10:
		func10_ptr = (func10)function_ptr->func_addr;
		ret = func10_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2), *(arg_ptr + 3), *(arg_ptr + 4), *(arg_ptr + 5), *(arg_ptr + 6), *(arg_ptr + 7), *(arg_ptr + 8), *(arg_ptr + 9));
		break;
	case 11:
		func11_ptr = (func11)function_ptr->func_addr;
		ret = func11_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2), *(arg_ptr + 3), *(arg_ptr + 4), *(arg_ptr + 5), *(arg_ptr + 6), *(arg_ptr + 7), *(arg_ptr + 8), *(arg_ptr + 9), *(arg_ptr + 10));
		break;
	case 12:
		func12_ptr = (func12)function_ptr->func_addr;
		ret = func12_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2), *(arg_ptr + 3), *(arg_ptr + 4), *(arg_ptr + 5), *(arg_ptr + 6), *(arg_ptr + 7), *(arg_ptr + 8), *(arg_ptr + 9), *(arg_ptr + 10), *(arg_ptr + 11));
		break;
	case 13:
		func13_ptr = (func13)function_ptr->func_addr;
		ret = func13_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2), *(arg_ptr + 3), *(arg_ptr + 4), *(arg_ptr + 5), *(arg_ptr + 6), *(arg_ptr + 7), *(arg_ptr + 8), *(arg_ptr + 9), *(arg_ptr + 10), *(arg_ptr + 11), *(arg_ptr + 12));
		break;
	case 14:
		func14_ptr = (func14)function_ptr->func_addr;
		ret = func14_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2), *(arg_ptr + 3), *(arg_ptr + 4), *(arg_ptr + 5), *(arg_ptr + 6), *(arg_ptr + 7), *(arg_ptr + 8), *(arg_ptr + 9), *(arg_ptr + 10), *(arg_ptr + 11), *(arg_ptr + 12), *(arg_ptr + 13));
		break;
	case 15:
		func15_ptr = (func15)function_ptr->func_addr;
		ret = func15_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2), *(arg_ptr + 3), *(arg_ptr + 4), *(arg_ptr + 5), *(arg_ptr + 6), *(arg_ptr + 7), *(arg_ptr + 8), *(arg_ptr + 9), *(arg_ptr + 10), *(arg_ptr + 11), *(arg_ptr + 12), *(arg_ptr + 13), *(arg_ptr + 14));
		break;
	case 16:
		func16_ptr = (func16)function_ptr->func_addr;
		ret = func16_ptr(*arg_ptr, *(arg_ptr + 1), *(arg_ptr + 2), *(arg_ptr + 3), *(arg_ptr + 4), *(arg_ptr + 5), *(arg_ptr + 6), *(arg_ptr + 7), *(arg_ptr + 8), *(arg_ptr + 9), *(arg_ptr + 10), *(arg_ptr + 11), *(arg_ptr + 12), *(arg_ptr + 13), *(arg_ptr + 14), *(arg_ptr + 15));
		break;
	default:
		error("Arg size cannot exceed 64B.\n");
		return ERROR_NO;
	}
	varity_convert(ret_addr, instruction_ptr->ret_varity_type, &ret, varity_get_type(function_ptr->arg_count - 2, function_ptr->arg));
	interpreter_ptr->cpsr.last_ret_addr = (PLATFORM_WORD*)ret_addr;
	interpreter_ptr->cpsr.last_type = instruction_ptr->ret_varity_type;
	return 0;
}

int c_interpreter::opt_func_comma_handle(c_interpreter *interpreter_ptr)//TODO: 可删，根本不会生成comma中间代码，生成的是passpara
{
	return opt_assign_handle(interpreter_ptr);
}

int c_interpreter::opt_exist_handle(c_interpreter *interpreter_ptr)
{
	mid_code *instruction_ptr = interpreter_ptr->pc;
	int i;
	unsigned int count;
	count = interpreter_ptr->function_declare->function_stack_ptr->get_count();
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	GET_OPDB_ADDR();
	GET_RET_ADDR();
	for(i=0; i<count; i++)
		if(!kstrcmp(((function_info*)interpreter_ptr->function_declare->function_stack_ptr->visit_element_by_index(i))->get_name(), (const char*)instruction_ptr->opdb_addr)) {
			INT_VALUE(ret_addr) = 1;
			return 0;
		}
	count = interpreter_ptr->varity_declare->global_varity_stack->get_count();
	for(i=0; i<count; i++)
		if(!kstrcmp(((varity_info*)interpreter_ptr->varity_declare->global_varity_stack->visit_element_by_index(i))->get_name(), (const char*)instruction_ptr->opdb_addr)) {
			INT_VALUE(ret_addr) = 1;
			return 0;
		}
	INT_VALUE(ret_addr) = 0;
	return 0;
}

ITCM_TEXT int c_interpreter::ctl_branch_handle(c_interpreter *interpreter_ptr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	instruction_ptr += instruction_ptr->opda_addr - 1;
	return ERROR_NO;
}

ITCM_TEXT int c_interpreter::ctl_branch_true_handle(c_interpreter *interpreter_ptr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	if(interpreter_ptr->cpsr.last_type < CHAR || interpreter_ptr->cpsr.last_type > U_LONG_LONG)
		return ERROR_CONDITION_TYPE;
	if(is_non_zero(interpreter_ptr->cpsr.last_type, interpreter_ptr->cpsr.last_ret_addr))
		instruction_ptr += instruction_ptr->opda_addr - 1;
	return ERROR_NO;
}

ITCM_TEXT int c_interpreter::ctl_branch_false_handle(c_interpreter *interpreter_ptr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	if(interpreter_ptr->cpsr.last_type < CHAR || interpreter_ptr->cpsr.last_type > U_LONG_LONG)
		return ERROR_CONDITION_TYPE;
	if(!is_non_zero(interpreter_ptr->cpsr.last_type, interpreter_ptr->cpsr.last_ret_addr))//TODO:确认又|| && :?等跳转指令过来的时候使用上一条的type是否合理
		instruction_ptr += instruction_ptr->opda_addr - 1;
	return ERROR_NO;
}

int c_interpreter::opt_pass_para_handle(c_interpreter *interpreter_ptr)
{
	return opt_assign_handle(interpreter_ptr);
}

int c_interpreter::ctl_return_handle(c_interpreter *interpreter_ptr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	instruction_ptr += instruction_ptr->opda_addr - 1;
	return ERROR_NO;
}

int c_interpreter::ctl_bxlr_handle(c_interpreter *interpreter_ptr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
	mid_code *this_instruction_ptr = instruction_ptr;
	instruction_ptr = (mid_code*)PTR_N_VALUE(interpreter_ptr->stack_pointer - PLATFORM_WORD_LEN);
	char *&sp = interpreter_ptr->stack_pointer, *&t_varity_sp = interpreter_ptr->tmp_varity_stack_pointer;
	t_varity_sp += (int)instruction_ptr->opdb_addr;//24;
	GET_RET_ADDR();
	//varity_convert(ret_addr, instruction_ptr->ret_varity_type, t_varity_sp + instruction_ptr->opdb_addr - 8, this_instruction_ptr->opdb_addr);
	varity_convert(ret_addr, instruction_ptr->ret_varity_type, t_varity_sp - instruction_ptr->ret_addr - 8, this_instruction_ptr->opdb_addr);
	return ERROR_NO;
}

int c_interpreter::ctl_sp_add_handle(c_interpreter *interpreter_ptr)
{
	mid_code *&instruction_ptr = interpreter_ptr->pc;
#if STACK_OVERFLOW_CHECK
	if(interpreter_ptr->stack_pointer + (int)instruction_ptr->opda_addr >= interpreter_ptr->tmp_varity_stack_pointer) {
		error("Stack overflow.\n");
		return ERROR_STACK_OVERFLOW;
	}
#endif
	interpreter_ptr->stack_pointer += (int)instruction_ptr->opda_addr;
	return ERROR_NO;
}

void c_interpreter::handle_init(void)
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
	opt_handle[OPT_DEVIDE_ASSIGN] = opt_devide_assign_handle;
	opt_handle[OPT_MUL_ASSIGN] = opt_mul_assign_handle;
	opt_handle[OPT_MOD_ASSIGN] = opt_mod_assign_handle;
	opt_handle[OPT_ADD_ASSIGN] = opt_add_assign_handle;
	opt_handle[OPT_MINUS_ASSIGN] = opt_minus_assign_handle;
	opt_handle[OPT_BIT_AND_ASSIGN] = opt_bit_and_assign_handle;
	opt_handle[OPT_BIT_XOR_ASSIGN] = opt_xor_assign_handle;
	opt_handle[OPT_BIT_OR_ASSIGN] = opt_bit_or_assign_handle;
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
	opt_handle[OPT_EXIST] = opt_exist_handle;
	opt_handle[CTL_BRANCH] = ctl_branch_handle;
	opt_handle[CTL_BRANCH_TRUE] = ctl_branch_true_handle;
	opt_handle[CTL_BRANCH_FALSE] = ctl_branch_false_handle;
	opt_handle[OPT_PASS_PARA] = opt_pass_para_handle;
	opt_handle[CTL_RETURN] = ctl_return_handle;
	opt_handle[CTL_BREAK] = ctl_branch_handle;
	opt_handle[CTL_CONTINUE] = ctl_branch_handle;
	opt_handle[CTL_GOTO] = ctl_branch_handle;
	opt_handle[CTL_BXLR] = ctl_bxlr_handle;
	opt_handle[CTL_SP_ADD] = ctl_sp_add_handle;
}

int opt_time;
int c_interpreter::call_opt_handle(c_interpreter *interpreter_ptr)
{
	mid_code *instruction_ptr = interpreter_ptr->pc;
	int ret;
	int tick1, tick2;
	if(interpreter_ptr->break_flag) {
		error("Interrupted by break.\n");
		interpreter_ptr->break_flag = 0;
		return ERROR_CTL_BREAK;
	}
#if DEBUG_EN
	int gdbret;
	gdbret = gdb::breakpoint_handle(interpreter_ptr, instruction_ptr);
	if(gdbret < 0)
		return gdbret;
	if(gdb::get_trace_status())
		interpreter_ptr->print_code(instruction_ptr, 1, 1);
		//gdbout("addr=%x,opt=%d\n", instruction_ptr, instruction_ptr->ret_operator);
#endif
	if(opt_handle[instruction_ptr->ret_operator]) {
		//tick1 = HWREG(0x2040018);
		ret = opt_handle[instruction_ptr->ret_operator](interpreter_ptr);
		//if(gdb::get_trace_status())
		//	kprintf("ret=%08x\n", *interpreter_ptr->cpsr.last_ret_addr);
		//tick2 = HWREG(0x2040018);
		//opt_time += tick1 - tick2;
#if DEBUG_EN
		if(instruction_ptr == gdb::get_bp_todo()) {
			(interpreter_ptr->pc + 1)->break_flag |= BREAKPOINT_STEP;
			gdb::clear_bp_todo();
		}
#endif
	} else {
		error("Undefined instruction. opt: %d\n", instruction_ptr->ret_operator);
		ret = ERROR_UNDEFINED_OPT;
	}
	return ret;
}

int try_assign_handle(int opda_type, int opdb_type, int opda_complex_count, int *opda_type_info, int opdb_complex_count, int *opdb_type_info)
{
	if(opda_type == PTR) {
		if(opdb_type != PTR && opdb_type != ARRAY)
			goto wrong;
		else {
			if(opda_complex_count == 2 && opda_type_info[1] == BASIC_TYPE_SET(VOID) && GET_COMPLEX_TYPE(opda_type_info[2]) == COMPLEX_PTR)
				return PTR;
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
	} else if(opda_type == ARRAY) {
		goto wrong;
	} else {
		if(opdb_type < CHAR || opdb_type > DOUBLE)
			goto wrong;
	}
	return opda_type;
wrong:
	//error("Can't assign.\n");
	return ERROR_ILLEGAL_OPERAND;
}

int try_plus_handle(int opda_type, int opdb_type, int opda_complex_count, int *opda_type_info, int opdb_complex_count, int *opdb_type_info)
{
	int ret_type;
	if(opda_type < VOID && opdb_type < VOID) {
		ret_type = opda_type < opdb_type ? opdb_type : opda_type;
		if(ret_type < INT)
			ret_type = INT;
		return ret_type;
	} else if(opda_type == STRUCT || opdb_type == STRUCT || opda_type == VOID || opdb_type == VOID) {
		return ERROR_ILLEGAL_OPERAND;
	} else if(opda_type >= PTR && opdb_type < VOID || opda_type < VOID && opdb_type >= PTR) {
		return PTR;
	} else
		return ERROR_ILLEGAL_OPERAND;
}

int try_minus_handle(int opda_type, int opdb_type, int opda_complex_count, int *opda_type_info, int opdb_complex_count, int *opdb_type_info)
{
	if(opda_type < VOID && opdb_type < VOID) {
		return opda_type<opdb_type?opdb_type:opda_type;
	} else if(opda_type == STRUCT || opdb_type == STRUCT || opda_type == VOID || opdb_type == VOID) {
		goto wrong;
	} else if(opda_type >= PTR && opdb_type <= U_LONG_LONG) {
		return PTR;
	} else if(opda_type >= PTR && opdb_type >= PTR) {
		if(opda_complex_count != opdb_complex_count)
			goto wrong;
		else {
			for(int i=1; i<=opda_complex_count-1; i++) {
				if(opda_type_info[i] != opdb_type_info[i])
					goto wrong;
			}
		}
		return INT;
	} else
		goto wrong;
	return ERROR_NO;
wrong:
	//error("Can't minus.\n");
	return ERROR_ILLEGAL_OPERAND;
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
	//error("Can't compare.\n");
	return ERROR_ILLEGAL_OPERAND;
}

int try_mod_handle(int opda_type, int opdb_type, int opda_complex_count, int *opda_type_info, int opdb_complex_count, int *opdb_type_info)
{
	int ret_type;
	if(opda_type <= U_LONG_LONG && opdb_type <= U_LONG_LONG) {
		ret_type = opda_type < opdb_type ? opdb_type : opda_type;
		if(ret_type < INT)
			ret_type = INT;
		return ret_type;
	} else
		return ERROR_ILLEGAL_OPERAND;
}

int try_call_opt_handle(int opt, mid_code* instruction_ptr, void *avarity_ptr, void *bvarity_ptr)
{
	int opda_type = instruction_ptr->opda_varity_type;
	int opdb_type = instruction_ptr->opdb_varity_type;
	int opda_complex_count = ((varity_info*)avarity_ptr)->get_complex_arg_count();
	int opdb_complex_count = ((varity_info*)bvarity_ptr)->get_complex_arg_count();
	int *opda_type_info = (int*)((varity_info*)avarity_ptr)->get_complex_ptr();
	int *opdb_type_info = (int*)((varity_info*)bvarity_ptr)->get_complex_ptr();
	if(instruction_ptr->opdb_operand_type == OPERAND_FUNCTION)
		return ERROR_NO;
	switch(opt) {
	case OPT_ASSIGN:
		return try_assign_handle(opda_type, opdb_type, opda_complex_count, opda_type_info, opdb_complex_count, opdb_type_info);
	case OPT_PLUS:
		return try_plus_handle(opda_type, opdb_type, opda_complex_count, opda_type_info, opdb_complex_count, opdb_type_info);
	case OPT_MINUS:
		return try_minus_handle(opda_type, opdb_type, opda_complex_count, opda_type_info, opdb_complex_count, opdb_type_info);
	case OPT_MUL:
		return try_mul_handle(opda_type, opdb_type, opda_complex_count, opda_type_info, opdb_complex_count, opdb_type_info);
	case OPT_EQU:
		return try_compare_handle(opda_type, opdb_type, opda_complex_count, opda_type_info, opdb_complex_count, opdb_type_info);
	case OPT_MOD:
		return try_mod_handle(opda_type, opdb_type, opda_complex_count, opda_type_info, opdb_complex_count, opdb_type_info);
	}
	return ERROR_NO;
}
