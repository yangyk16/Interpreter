#include "interpreter.h"
#include "operator.h"
#include "string_lib.h"
#include "error.h"
#include "varity.h"
#include "data_struct.h"
#include "cstdlib.h"
#include "kmalloc.h"

#if TTY_TYPE == 0
tty stdio;
#elif TTY_TYPE == 1
uart stdio;
#endif

varity_type_stack_t c_interpreter::varity_type_stack;
language_elment_space_t c_interpreter::language_elment_space;
c_interpreter myinterpreter;

char non_seq_key[][7] = {"", "if", "switch", "else", "for", "while", "do"};
const char non_seq_key_len[] = {0, 2, 6, 4, 3, 5, 2};
char opt_str[43][4] = {"<<=",">>=","->","++","--","<<",">>",">=","<=","==","!=","&&","||","/=","*=","%=","+=","-=","&=","^=","|=","[","]","(",")",".","-","~","*","&","!","/","%","+",">","<","^","|","?",":","=",",",";"};
char opt_str_len[] = {3,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
char opt_prio[] ={14,14,1,2,2,5,5,6,6,7,7,11,12,14,14,14,14,14,14,14,14,1,1,1,17,1,4,2,3,8,2,3,3,4,6,6,9,10,13,13,14,15,16};
char opt_number[] = {2,2,2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,2,2,2,1,1,1,1,1,1};
char tmp_varity_name[MAX_A_VARITY_NODE][3];
char link_varity_name[MAX_A_VARITY_NODE][3];

int c_interpreter::list_stack_to_tree(node* tree_node, list_stack* post_order_stack)
{
	node *last_node;
	node_attribute_t *last_node_attribute;
	int ret;
	if(!tree_node->right) {
		last_node = (node*)post_order_stack->pop();
		if(!last_node) {
			error("Operand insufficient.\n");
			return ERROR_OPERAND_LACKED;
		}
		last_node->link_reset();
		last_node_attribute = (node_attribute_t*)last_node->value;
		tree_node->right = last_node;
		if(last_node_attribute->node_type == TOKEN_OPERATOR) {
			if(last_node_attribute->data == OPT_TYPE_CONVERT && ((node_attribute_t*)tree_node->value)->data == OPT_SIZEOF) {
				return ERROR_NO;
			}
			ret = list_stack_to_tree(last_node, post_order_stack);
			if(ret)
				return ret;
		} else if(last_node_attribute->node_type == TOKEN_NAME || last_node_attribute->node_type == TOKEN_CONST_VALUE || last_node_attribute->node_type == TOKEN_STRING) {
		} else {
			error("Invalid key word.\n");
			return ERROR_INVALID_KEYWORD;
		}
		if(((node_attribute_t*)tree_node->value)->node_type == TOKEN_OPERATOR && opt_number[((node_attribute_t*)tree_node->value)->data] == 1) {//单目运算符
			return ERROR_NO;
		}
		if(((node_attribute_t*)tree_node->value)->node_type == TOKEN_OPERATOR && ((node_attribute_t*)tree_node->value)->data == OPT_CALL_FUNC) {
			function_info *function_ptr = this->function_declare->find(((node_attribute_t*)tree_node->value - 1)->value.ptr_value);
			if(!function_ptr) {
				error("Function not found.\n");
				return ERROR_NO_FUNCTION;
			}
			if(function_ptr->arg_list->get_count() == 1) {//无参数函数
				tree_node->left = last_node;
				tree_node->right = 0;
				return ERROR_NO;
			}
		}
	}
	if(!tree_node->left) {
		last_node = (node*)post_order_stack->pop();
		if(!last_node) {
			error("Operand insufficient.\n");
			return ERROR_OPERAND_LACKED;
		}
		last_node->link_reset();
		last_node_attribute = (node_attribute_t*)last_node->value;
		tree_node->left = last_node;
		if(last_node_attribute->node_type == TOKEN_OPERATOR) {
			ret = list_stack_to_tree(last_node, post_order_stack);
			if(ret)
				return ret;
		} else if(last_node_attribute->node_type == TOKEN_NAME || last_node_attribute->node_type == TOKEN_CONST_VALUE || last_node_attribute->node_type == TOKEN_STRING) {
		} else {
			error("Invalid key word.\n");
			return ERROR_INVALID_KEYWORD;
		}
	}
	return ERROR_NO;
	//if(tree_node->right && ((node_attribute_t*)tree_node->right->value)->node_type == TOKEN_NAME
	//	&& tree_node->left && ((node_attribute_t*)tree_node->left->value)->node_type == TOKEN_NAME) {
	//		return ERROR_NO;
	//}
}

#define RETURN(x) if(avarity_use_flag && !PTR_N_VALUE(avarity_ptr->get_complex_ptr())) \
		vfree(avarity_ptr->get_complex_ptr()); \
	if(bvarity_use_flag && !PTR_N_VALUE(bvarity_ptr->get_complex_ptr())) \
		vfree(bvarity_ptr->get_complex_ptr()); \
	return x
int c_interpreter::operator_post_handle(stack *code_stack_ptr, node *opt_node_ptr)
{
	varity_info static_varity;
	register varity_info *avarity_ptr = &static_varity, *bvarity_ptr = &static_varity, *rvarity_ptr;
	int opt = ((node_attribute_t*)opt_node_ptr->value)->data;
	int varity_scope;
	register node_attribute_t *node_attribute;
	register mid_code *instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
	int varity_number;
	bool avarity_use_flag = 0, bvarity_use_flag = 0;
	void *avarity_info = 0, *bvarity_info = 0;
	if(opt_number[opt] > 1) {
		node_attribute = (node_attribute_t*)opt_node_ptr->left->value;//判断是否是双目，单目不能看左树
		if(node_attribute->node_type == TOKEN_CONST_VALUE) {
			instruction_ptr->opda_operand_type = OPERAND_CONST;
			instruction_ptr->opda_varity_type = node_attribute->value_type;
			kmemcpy(&instruction_ptr->opda_addr, &node_attribute->value, 8);
		} else if(node_attribute->node_type == TOKEN_STRING) {
			instruction_ptr->opda_operand_type = OPERAND_G_VARITY;
			instruction_ptr->opda_varity_type = ARRAY;
			kmemcpy(&instruction_ptr->opda_addr, &node_attribute->value, 8);
		} else if(node_attribute->node_type == TOKEN_NAME) {
			if(node_attribute->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {
				instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
				instruction_ptr->opda_addr = 8 * node_attribute->value.ptr_value[1];
				avarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]);
				instruction_ptr->opda_varity_type = avarity_ptr->get_type();
				this->mid_varity_stack.pop();
				avarity_use_flag = 1;
				dec_varity_ref(avarity_ptr, false);
			} else if(node_attribute->value.ptr_value[0] == LINK_VARITY_PREFIX) {
				varity_number = node_attribute->value.ptr_value[1];
				avarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
				instruction_ptr->opda_operand_type = OPERAND_LINK_VARITY;
				instruction_ptr->opda_varity_type = avarity_ptr->get_type();
				instruction_ptr->opda_addr = 8 * node_attribute->value.ptr_value[1];
				this->mid_varity_stack.pop();
				avarity_use_flag = 1;
				dec_varity_ref(avarity_ptr, false);
			} else {
				if(opt != OPT_CALL_FUNC) {
					avarity_ptr = this->varity_declare->vfind(node_attribute->value.ptr_value, varity_scope);
					if(!avarity_ptr) {
						error("Varity not exist.\n");
						return ERROR_VARITY_NONEXIST;
					}
					if(varity_scope == VARITY_SCOPE_GLOBAL)
						instruction_ptr->opda_operand_type = OPERAND_G_VARITY;
					else
						instruction_ptr->opda_operand_type = OPERAND_L_VARITY;
					instruction_ptr->opda_addr = (int)avarity_ptr->get_content_ptr();
					instruction_ptr->opda_varity_type = avarity_ptr->get_type();
				}
			}
		}
	}
	if(opt_node_ptr->right) {
		node_attribute = (node_attribute_t*)opt_node_ptr->right->value;
		if(node_attribute->node_type == TOKEN_CONST_VALUE) {
			instruction_ptr->opdb_operand_type = OPERAND_CONST;
			instruction_ptr->opdb_varity_type = node_attribute->value_type;
			kmemcpy(&instruction_ptr->opdb_addr, &node_attribute->value, 8);
		} else if(node_attribute->node_type == TOKEN_STRING) {
			instruction_ptr->opdb_operand_type = OPERAND_G_VARITY;
			instruction_ptr->opdb_varity_type = ARRAY;
			kmemcpy(&instruction_ptr->opdb_addr, &node_attribute->value, 8);
		} else if(node_attribute->node_type == TOKEN_NAME) {
			if(node_attribute->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {
				instruction_ptr->opdb_operand_type = OPERAND_T_VARITY;
				instruction_ptr->opdb_addr = 8 * node_attribute->value.ptr_value[1];
				bvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]);
				instruction_ptr->opdb_varity_type = bvarity_ptr->get_type();
				this->mid_varity_stack.pop();
				bvarity_use_flag = 1;
				dec_varity_ref(bvarity_ptr, false);
			} else if(node_attribute->value.ptr_value[0] == LINK_VARITY_PREFIX) {
				instruction_ptr->opdb_operand_type = OPERAND_LINK_VARITY;
				bvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]);
				instruction_ptr->opdb_varity_type = bvarity_ptr->get_type();
				instruction_ptr->opdb_addr = 8 * node_attribute->value.ptr_value[1];
				this->mid_varity_stack.pop();
				bvarity_use_flag = 1;
				dec_varity_ref(bvarity_ptr, false);
			} else {
				if(opt != OPT_MEMBER && opt != OPT_REFERENCE) {
					bvarity_ptr = this->varity_declare->vfind(node_attribute->value.ptr_value, varity_scope);
					if(!bvarity_ptr) {
						error("Varity not exist.\n");
						return ERROR_VARITY_NONEXIST;
					}
					if(varity_scope == VARITY_SCOPE_GLOBAL)
						instruction_ptr->opdb_operand_type = OPERAND_G_VARITY;
					else
						instruction_ptr->opdb_operand_type = OPERAND_L_VARITY;
					instruction_ptr->opdb_addr = (int)bvarity_ptr->get_content_ptr();
					instruction_ptr->opdb_varity_type = bvarity_ptr->get_type();
				}
			}
		}
	}

	switch(opt) {
		int ret_type;
	case OPT_MEMBER:
	case OPT_REFERENCE:
	{
		varity_info *member_varity_ptr, *struct_ptr = avarity_ptr;
		struct_info *struct_info_ptr = (struct_info*)(((PLATFORM_WORD*)avarity_ptr->get_complex_ptr())[1]);
		if(opt == OPT_MEMBER) {
			if(avarity_ptr->get_type() != STRUCT) {
				error("Only struct can use member operator.\n");
				RETURN(ERROR_ILLEGAL_OPERAND);
			}
		} else {
			if(avarity_ptr->get_complex_arg_count() != 3 || GET_COMPLEX_DATA(((PLATFORM_WORD*)avarity_ptr->get_complex_ptr())[2]) != STRUCT || GET_COMPLEX_TYPE(((PLATFORM_WORD*)avarity_ptr->get_complex_ptr())[3]) != COMPLEX_PTR) {
				error("Only pointer to struct can use reference operator.\n");
				RETURN(ERROR_ILLEGAL_OPERAND);
			}
		}
		//ret_type = get_ret_type(instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type);
		varity_number = this->mid_varity_stack.get_count();
		member_varity_ptr = (varity_info*)struct_info_ptr->varity_stack_ptr->find(node_attribute->value.ptr_value);
		if(!member_varity_ptr) {
			error("No member in struct.\n");
			RETURN(ERROR_STRUCT_MEMBER);
		}
		avarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		avarity_ptr->config_complex_info(member_varity_ptr->get_complex_arg_count(), member_varity_ptr->get_complex_ptr());
		//avarity_ptr->set_type(member_varity_ptr->get_type());
		this->mid_varity_stack.push();
		inc_varity_ref(avarity_ptr);
		if(opt == OPT_MEMBER) {
			instruction_ptr->opda_varity_type = INT;
			instruction_ptr->opdb_addr = (int)member_varity_ptr->get_content_ptr();
			instruction_ptr->opdb_operand_type = OPERAND_CONST;
			instruction_ptr->opdb_varity_type = INT;
			instruction_ptr->ret_operator = opt;
			instruction_ptr->ret_addr = varity_number * 8;
			instruction_ptr->ret_varity_type = INT;
			instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		} else if(opt == OPT_REFERENCE) {
			instruction_ptr->opda_varity_type = INT;
			instruction_ptr->opdb_addr = (int)member_varity_ptr->get_content_ptr();
			instruction_ptr->opdb_operand_type = OPERAND_CONST;
			instruction_ptr->opdb_varity_type = INT;
			instruction_ptr->ret_operator = OPT_PLUS;
			instruction_ptr->ret_addr = varity_number * 8;
			instruction_ptr->ret_varity_type = INT;
			instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		}
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = link_varity_name[varity_number];
		((node_attribute_t*)opt_node_ptr->value)->value_type = VARITY_SCOPE_ANALYSIS;
		break;
	}
	case OPT_INDEX:
	{//TODO:set_type也需要重写与声明统一
		varity_number = this->mid_varity_stack.get_count();
		PLATFORM_WORD *complex_info_ptr = avarity_ptr->get_complex_ptr();
		int complex_arg_count = avarity_ptr->get_complex_arg_count();
		if(GET_COMPLEX_TYPE(complex_info_ptr[complex_arg_count]) == COMPLEX_ARRAY || GET_COMPLEX_TYPE(complex_info_ptr[complex_arg_count]) == COMPLEX_PTR) {
			rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
			rvarity_ptr->set_size(get_varity_size(GET_COMPLEX_DATA(complex_info_ptr[0]), complex_info_ptr, complex_arg_count - 1));
			rvarity_ptr->config_complex_info(complex_arg_count - 1, complex_info_ptr);
			this->mid_varity_stack.push();
			inc_varity_ref(rvarity_ptr);
			instruction_ptr->data = get_varity_size(0, complex_info_ptr, complex_arg_count - 1);
			instruction_ptr->ret_addr = varity_number * 8;
			instruction_ptr->ret_varity_type = INT;
			instruction_ptr->ret_operand_type = OPERAND_LINK_VARITY;
			instruction_ptr->ret_operator = opt;
			((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
			((node_attribute_t*)opt_node_ptr->value)->value_type = instruction_ptr->opda_operand_type;
			((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = link_varity_name[varity_number];
		} else {
			error("No array or ptr varity for using [].\n");
			RETURN(ERROR_USED_INDEX);
		}
		break;
	}
	case OPT_PLUS:
	case OPT_MINUS:
	case OPT_MUL:
	case OPT_DIVIDE:
		if(opt == OPT_PLUS && instruction_ptr->opda_varity_type >= CHAR && instruction_ptr->opda_varity_type <= U_LONG_LONG && instruction_ptr->opdb_varity_type <= ARRAY && instruction_ptr->opdb_varity_type >= PTR) {
			char tmp[sizeof(instruction_ptr->opda_addr) + sizeof(instruction_ptr->opda_operand_type) + sizeof(instruction_ptr->double_space1) + sizeof(instruction_ptr->opda_varity_type)];
			kmemcpy(tmp, &instruction_ptr->opda_addr, sizeof(tmp));
			kmemcpy(&instruction_ptr->opda_addr, &instruction_ptr->opdb_addr, sizeof(tmp));
			kmemcpy(&instruction_ptr->opdb_addr, tmp, sizeof(tmp));
		}
		if(opt == OPT_PLUS)
			ret_type = try_call_opt_handle(OPT_PLUS, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, avarity_ptr->get_complex_arg_count(), (int*)avarity_ptr->get_complex_ptr(), bvarity_ptr->get_complex_arg_count(), (int*)bvarity_ptr->get_complex_ptr());
		else if(opt == OPT_MINUS)//减法可用两个指针减，改用cmp的try_handle
			ret_type = try_call_opt_handle(OPT_MINUS, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, avarity_ptr->get_complex_arg_count(), (int*)avarity_ptr->get_complex_ptr(), bvarity_ptr->get_complex_arg_count(), (int*)bvarity_ptr->get_complex_ptr());
		else
			ret_type = try_call_opt_handle(OPT_MUL, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, avarity_ptr->get_complex_arg_count(), (int*)avarity_ptr->get_complex_ptr(), bvarity_ptr->get_complex_arg_count(), (int*)bvarity_ptr->get_complex_ptr());
		if(ret_type < 0) {
			RETURN(ret_type);
		}
		varity_number = this->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(ret_type);
		if(ret_type == PTR || ret_type == INT) {
			if(instruction_ptr->opda_varity_type >= PTR && instruction_ptr->opdb_varity_type <= VOID) {
				rvarity_ptr->config_complex_info(avarity_ptr->get_complex_arg_count(), avarity_ptr->get_complex_ptr());
				instruction_ptr->data = avarity_ptr->get_first_order_sub_struct_size();
			} else if(instruction_ptr->opda_varity_type <= VOID && instruction_ptr->opdb_varity_type >= PTR) {
				rvarity_ptr->config_complex_info(bvarity_ptr->get_complex_arg_count(), bvarity_ptr->get_complex_ptr());
				instruction_ptr->data = bvarity_ptr->get_first_order_sub_struct_size();
			} else if(instruction_ptr->opda_varity_type >= PTR && instruction_ptr->opdb_varity_type >= PTR) {
				rvarity_ptr->config_complex_info(bvarity_ptr->get_complex_arg_count(), bvarity_ptr->get_complex_ptr());
				instruction_ptr->data = bvarity_ptr->get_first_order_sub_struct_size();
			}
			array_to_ptr((PLATFORM_WORD*&)rvarity_ptr->get_complex_ptr(), rvarity_ptr->get_complex_arg_count());
		}
		this->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = get_ret_type(instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type);
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		break;
	case OPT_EQU:
	case OPT_NOT_EQU:
	case OPT_BIG_EQU:
	case OPT_SMALL_EQU:
	case OPT_BIG:
	case OPT_SMALL:
		ret_type = try_call_opt_handle(OPT_EQU, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, avarity_ptr->get_complex_arg_count(), (int*)avarity_ptr->get_complex_ptr(), bvarity_ptr->get_complex_arg_count(), (int*)bvarity_ptr->get_complex_ptr());
		if(ret_type < 0) {
			RETURN(ret_type);
		}
		ret_type = INT;
		varity_number = this->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(ret_type);
		this->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = INT;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		break;
	case OPT_ASL_ASSIGN:
	case OPT_ASR_ASSIGN:
	case OPT_MOD_ASSIGN:
	case OPT_BIT_AND_ASSIGN:
	case OPT_BIT_XOR_ASSIGN:
	case OPT_BIT_OR_ASSIGN:
		if(instruction_ptr->opda_varity_type > U_LONG_LONG || instruction_ptr->opdb_varity_type > U_LONG_LONG) {
			error("Need int type operand.\n");
			RETURN(ERROR_ILLEGAL_OPERAND);
		}
		goto assign_general;
	case OPT_ASSIGN:
		ret_type = try_call_opt_handle(OPT_ASSIGN, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, avarity_ptr->get_complex_arg_count(), (int*)avarity_ptr->get_complex_ptr(), bvarity_ptr->get_complex_arg_count(), (int*)bvarity_ptr->get_complex_ptr());
		if(ret_type < 0) {
			RETURN(ret_type);
		}
		goto assign_general;
	case OPT_MUL_ASSIGN:
	case OPT_DEVIDE_ASSIGN:
		ret_type = try_call_opt_handle(OPT_MUL, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, avarity_ptr->get_complex_arg_count(), (int*)avarity_ptr->get_complex_ptr(), bvarity_ptr->get_complex_arg_count(), (int*)bvarity_ptr->get_complex_ptr());
		if(ret_type < 0) {
			RETURN(ret_type);
		}
		goto assign_general;
	case OPT_ADD_ASSIGN:
		ret_type = try_call_opt_handle(OPT_PLUS, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, avarity_ptr->get_complex_arg_count(), (int*)avarity_ptr->get_complex_ptr(), bvarity_ptr->get_complex_arg_count(), (int*)bvarity_ptr->get_complex_ptr());
		if(ret_type < 0) {
			RETURN(ret_type);
		}
		if(ret_type == PTR) {
			if(instruction_ptr->opda_varity_type == PTR)
				goto assign_general;
			else {
				error("Can't plus assign.\n"); 
				RETURN(ret_type);
			}
		} else
			goto assign_general;
	case OPT_MINUS_ASSIGN:
		ret_type = try_call_opt_handle(OPT_MINUS, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, avarity_ptr->get_complex_arg_count(), (int*)avarity_ptr->get_complex_ptr(), bvarity_ptr->get_complex_arg_count(), (int*)bvarity_ptr->get_complex_ptr());
		if(ret_type < 0) {
			RETURN(ret_type);
		}
		if(ret_type == instruction_ptr->opda_varity_type)
			ret_type = try_call_opt_handle(OPT_ASSIGN, instruction_ptr->opda_varity_type, ret_type, avarity_ptr->get_complex_arg_count(), (int*)avarity_ptr->get_complex_ptr(), avarity_ptr->get_complex_arg_count(), (int*)avarity_ptr->get_complex_ptr());
		else
			ret_type = try_call_opt_handle(OPT_ASSIGN, instruction_ptr->opda_varity_type, ret_type, avarity_ptr->get_complex_arg_count(), (int*)avarity_ptr->get_complex_ptr(), bvarity_ptr->get_complex_arg_count(), (int*)bvarity_ptr->get_complex_ptr());
		if(ret_type < 0) {
			RETURN(ret_type);
		}
		goto assign_general;
assign_general:
		if(instruction_ptr->opda_operand_type == OPERAND_T_VARITY) {
			error("Assign operator need left value.\n");
			RETURN(ERROR_NEED_LEFT_VALUE);
		} else if(instruction_ptr->opda_operand_type == OPERAND_CONST) {
			error("Const value assigned.\n");
			RETURN(ERROR_CONST_ASSIGNED);
		} else {
		}
		if(ret_type == PTR || ret_type == INT) {
			if(instruction_ptr->opda_varity_type >= PTR && instruction_ptr->opdb_varity_type <= VOID) {
				instruction_ptr->data = avarity_ptr->get_first_order_sub_struct_size();
			} else if(instruction_ptr->opda_varity_type <= VOID && instruction_ptr->opdb_varity_type >= PTR) {
				instruction_ptr->data = bvarity_ptr->get_first_order_sub_struct_size();
			} else if(instruction_ptr->opda_varity_type >= PTR && instruction_ptr->opdb_varity_type >= PTR) {
				instruction_ptr->data = bvarity_ptr->get_first_order_sub_struct_size();
			}
		}
		instruction_ptr->ret_addr = instruction_ptr->opda_addr;
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type;
		instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = ((node_attribute_t*)opt_node_ptr->left->value)->value.ptr_value;
		if(instruction_ptr->opda_operand_type == OPERAND_T_VARITY || instruction_ptr->opda_operand_type == OPERAND_LINK_VARITY) {
			inc_varity_ref(avarity_ptr);
			this->mid_varity_stack.push();
		}
		break;
	case OPT_L_PLUS_PLUS:
	case OPT_L_MINUS_MINUS:
		if(instruction_ptr->opdb_operand_type == OPERAND_T_VARITY) {
			error("++ operator need left value.\n");
			RETURN(ERROR_NEED_LEFT_VALUE);
		}
		instruction_ptr->opda_addr = instruction_ptr->opdb_addr;
		instruction_ptr->opda_operand_type = instruction_ptr->opdb_operand_type;
		instruction_ptr->opda_varity_type = instruction_ptr->opdb_varity_type;
		instruction_ptr->ret_addr = instruction_ptr->opda_addr;
		instruction_ptr->ret_operator = (opt == OPT_L_PLUS_PLUS) ? OPT_PLUS : OPT_MINUS;
		instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type;
		instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type;
		instruction_ptr->opdb_addr = 1;
		instruction_ptr->opdb_operand_type = OPERAND_CONST;
		instruction_ptr->opdb_varity_type = INT;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = ((node_attribute_t*)opt_node_ptr->right->value)->value.ptr_value;
		break;
	case OPT_R_PLUS_PLUS:
	case OPT_R_MINUS_MINUS:
		if(instruction_ptr->opdb_operand_type == OPERAND_T_VARITY) {
			error("++ operator need left value.\n");
			RETURN(ERROR_NEED_LEFT_VALUE);
		}
		if(instruction_ptr->opdb_operand_type == OPERAND_CONST) {
			error("++ operator cannot used for const.\n");
			RETURN(ERROR_CONST_ASSIGNED);
		}
		varity_number = this->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->config_complex_info(bvarity_ptr->get_complex_arg_count(), bvarity_ptr->get_complex_ptr());
		this->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = instruction_ptr->opda_addr = varity_number * 8;
		instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = instruction_ptr->opdb_varity_type;
		instruction_ptr->ret_operator = OPT_ASSIGN;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		code_stack_ptr->push();

		(instruction_ptr + 1)->ret_addr = instruction_ptr->opdb_addr;
		(instruction_ptr + 1)->ret_operator = (opt == OPT_R_PLUS_PLUS) ? OPT_PLUS : OPT_MINUS;
		(instruction_ptr + 1)->ret_operand_type = instruction_ptr->opdb_operand_type;
		(instruction_ptr + 1)->ret_varity_type = instruction_ptr->opdb_varity_type;
		(instruction_ptr + 1)->opda_addr = instruction_ptr->opdb_addr;
		(instruction_ptr + 1)->opda_operand_type = instruction_ptr->opdb_operand_type;
		(instruction_ptr + 1)->opda_varity_type = instruction_ptr->opdb_varity_type;
		(instruction_ptr + 1)->opdb_addr = 1;
		(instruction_ptr + 1)->opdb_operand_type = OPERAND_CONST;
		(instruction_ptr + 1)->opdb_varity_type = INT;
		break;
	case OPT_ADDRESS_OF://TODO:取址时先验证扩展位是否为0再扩展，否则覆盖其他变量类型。
	{
		varity_number = this->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = PTR;
		instruction_ptr->ret_operator = opt;
		register PLATFORM_WORD* old_complex_info = (PLATFORM_WORD*)bvarity_ptr->get_complex_ptr();
		register int complex_arg_count = bvarity_ptr->get_complex_arg_count();
		if(GET_COMPLEX_TYPE(((uint*)bvarity_ptr->get_complex_ptr())[complex_arg_count + 1]) == COMPLEX_PTR) {
			rvarity_ptr->config_complex_info(complex_arg_count + 1, old_complex_info);
		} else {
			PLATFORM_WORD* new_complex_info = (PLATFORM_WORD*)vmalloc((complex_arg_count + 2) * sizeof(PLATFORM_WORD_LEN));
			kmemcpy(new_complex_info + 1, old_complex_info + 1, complex_arg_count * sizeof(PLATFORM_WORD_LEN));
			new_complex_info[complex_arg_count + 1] = COMPLEX_PTR << COMPLEX_TYPE_BIT;
			rvarity_ptr->config_complex_info(complex_arg_count + 1, new_complex_info);
		}
		this->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		break;
	}
	case OPT_PTR_CONTENT:
	{
		varity_number = this->mid_varity_stack.get_count();
		//bvarity被rvarity覆盖，先处理，都要留意获取rvarity之后的覆盖问题
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->config_complex_info(bvarity_ptr->get_complex_arg_count() - 1, bvarity_ptr->get_complex_ptr());
		rvarity_ptr->set_size(get_varity_size(0, bvarity_ptr->get_complex_ptr(), rvarity_ptr->get_complex_arg_count()));
		this->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_varity_type = rvarity_ptr->get_type();//TODO：考察所有指针在生成中间代码时置为void*的可行性
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = OPERAND_LINK_VARITY;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = link_varity_name[varity_number];
		break;
	}
	case OPT_BIT_AND:
	case OPT_BIT_OR:
	case OPT_BIT_XOR:
	case OPT_MOD:
	case OPT_ASL:
	case OPT_ASR:
		ret_type = try_call_opt_handle(OPT_MOD, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, avarity_ptr->get_complex_arg_count(), (int*)avarity_ptr->get_complex_ptr(), bvarity_ptr->get_complex_arg_count(), (int*)bvarity_ptr->get_complex_ptr());
		if(ret_type < 0) {
			RETURN(ret_type);
		}
	case OPT_NOT:
	case OPT_BIT_REVERT:
	case OPT_POSITIVE:
	case OPT_NEGATIVE:
		varity_number = this->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		if(opt == OPT_POSITIVE || opt == OPT_NEGATIVE) {
			rvarity_ptr->set_type(instruction_ptr->opdb_varity_type);
			instruction_ptr->ret_varity_type = instruction_ptr->opdb_varity_type;
		} else {
			rvarity_ptr->set_type(INT);
			instruction_ptr->ret_varity_type = INT;
		}
		this->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		break;
	case OPT_TERNARY_Q:
	{
		varity_number = this->mid_varity_stack.get_count();
		instruction_ptr->ret_addr = instruction_ptr->opda_addr = varity_number * 8;
		instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = instruction_ptr->opdb_varity_type;
		instruction_ptr->ret_operator = OPT_ASSIGN;
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(instruction_ptr->ret_varity_type);
		this->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		code_stack_ptr->push();
		instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
		instruction_ptr->ret_operator = CTL_BRANCH;
		mid_code *&ternary_code_ptr = (mid_code*&)this->sentence_analysis_data_struct.short_calc_stack[--this->sentence_analysis_data_struct.short_depth];
		ternary_code_ptr->opda_addr = ++instruction_ptr - ternary_code_ptr;
		this->sentence_analysis_data_struct.short_calc_stack[this->sentence_analysis_data_struct.short_depth] = (PLATFORM_WORD)(instruction_ptr - 1);
		break;
	}
	case OPT_TERNARY_C:
	{
		varity_number = this->mid_varity_stack.get_count();
		instruction_ptr->ret_addr = instruction_ptr->opda_addr = varity_number * 8;
		instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = instruction_ptr->opdb_varity_type;
		instruction_ptr->ret_operator = OPT_ASSIGN;
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(instruction_ptr->ret_varity_type);
		this->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		//code_stack_ptr->push();
		mid_code *&ternary_code_ptr = (mid_code*&)this->sentence_analysis_data_struct.short_calc_stack[this->sentence_analysis_data_struct.short_depth];
		ternary_code_ptr->opda_addr = instruction_ptr + 1 - ternary_code_ptr;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		((varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number))->set_type(instruction_ptr->ret_varity_type);
		break;
	}
	case OPT_AND:
	case OPT_OR:
	{
		ret_type = INT;
		varity_number = this->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(ret_type);
		this->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = INT;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		//code_stack_ptr->push();
		instruction_ptr = (mid_code*)this->sentence_analysis_data_struct.short_calc_stack[--this->sentence_analysis_data_struct.short_depth];
		instruction_ptr->opda_addr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr() - instruction_ptr + 1;
		(instruction_ptr - 1)->opda_addr = varity_number * 8;
		break;
	}
	case OPT_COMMA:
	{
		node_attribute_t *&root_attribute_ptr = (node_attribute_t*&)opt_node_ptr->value;
		root_attribute_ptr->node_type = node_attribute->node_type;
		root_attribute_ptr->value = node_attribute->value;
		root_attribute_ptr->value_type = node_attribute->value_type;
		RETURN(ERROR_NO);
	}
	case OPT_CALL_FUNC:
	case OPT_FUNC_COMMA:
		if(opt_node_ptr->right) {
			node_attribute = (node_attribute_t*)opt_node_ptr->right->value;
			if(node_attribute->node_type == TOKEN_NAME || node_attribute->node_type == TOKEN_CONST_VALUE || node_attribute->node_type == TOKEN_STRING) {
				stack *arg_list_ptr = (stack*)this->call_func_info.function_ptr[this->call_func_info.function_depth - 1]->arg_list;
				instruction_ptr->ret_operator = OPT_PASS_PARA;
				instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type = OPERAND_L_S_VARITY;//TODO:保证传进double时对齐到8Byte，否则data abort
#if CALL_CONVENTION
				instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
#endif
				if(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] >= this->call_func_info.arg_count[this->call_func_info.function_depth - 1] - 1) {
					if(!this->call_func_info.function_ptr[this->call_func_info.function_depth - 1]->variable_para_flag) {
						error("Too many parameters\n");
						RETURN(ERROR_FUNC_ARGS);
					} else {//可变参数
						if(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] == this->call_func_info.arg_count[this->call_func_info.function_depth - 1]) {
							this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
						}
						if(instruction_ptr->opdb_varity_type <= U_LONG && instruction_ptr->opdb_varity_type >= CHAR) {//TODO:平台相关，应该换掉
							instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = INT;
#if !CALL_CONVENTION
							instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4);
							this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4) + 4;
#else
							this->call_func_info.offset[this->call_func_info.function_depth - 1] += 4;
#endif
						} else if(instruction_ptr->opdb_varity_type == FLOAT || instruction_ptr->opdb_varity_type == DOUBLE) {
							instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = DOUBLE;
#if !CALL_CONVENTION
							instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8);
							this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + 8;
#else
							this->call_func_info.offset[this->call_func_info.function_depth - 1] += 8;
#endif
						} else if(instruction_ptr->opdb_varity_type == PTR || instruction_ptr->opdb_varity_type == ARRAY) {
							instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = PLATFORM_TYPE;
#if !CALL_CONVENTION
							instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
							this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) + PLATFORM_WORD_LEN;
#else
							this->call_func_info.offset[this->call_func_info.function_depth - 1] += PLATFORM_WORD_LEN;
#endif
						} else {
							instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = LONG_LONG;
#if !CALL_CONVENTION
							instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8);
							this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + 8;
#else
							this->call_func_info.offset[this->call_func_info.function_depth - 1] += 8;
#endif
						}
					}
				} else {//确定参数
					varity_info *arg_varity_ptr = (varity_info*)(arg_list_ptr->visit_element_by_index(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1]));
#if !CALL_CONVENTION
					switch(instruction_ptr->opdb_varity_type) {
					case DOUBLE:
					case LONG_LONG:
					case U_LONG_LONG:
						instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8);
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + arg_varity_ptr->get_size();
						break;
					case PTR:
					case ARRAY:
						instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) + arg_varity_ptr->get_size();
						break;
					default:
						instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4);
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4) + arg_varity_ptr->get_size();
						break;
					}
#else
					this->call_func_info.offset[this->call_func_info.function_depth - 1] += arg_varity_ptr->get_size();
#endif
					instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = arg_varity_ptr->get_type();
				}
				this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1]++;
				if(instruction_ptr->opdb_operand_type == OPERAND_T_VARITY || instruction_ptr->opdb_operand_type == OPERAND_LINK_VARITY) {
					//this->mid_varity_stack.pop();//TODO:加回来肯定有bug，不加回来不知道有没有
					//dec_varity_ref(bvarity_ptr, true);//TODO:确认是否有需要，不注释掉出bug了，释放了全局变量类型信息
				}
			}
		}
		if(opt == OPT_CALL_FUNC) {
			varity_info *return_varity_ptr;
			if(opt_node_ptr->right && !(node_attribute->data == OPT_FUNC_COMMA && node_attribute->node_type == TOKEN_OPERATOR))
				code_stack_ptr->push();
			function_info *function_ptr;
			instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
			node_attribute = (node_attribute_t*)opt_node_ptr->left->value;
			function_ptr = this->function_declare->find(node_attribute->value.ptr_value);
			return_varity_ptr = (varity_info*)function_ptr->arg_list->visit_element_by_index(0);
			instruction_ptr->opda_addr = (int)function_ptr;
			instruction_ptr->ret_operator = opt;
			instruction_ptr->ret_varity_type = return_varity_ptr->get_type();
			varity_number = this->mid_varity_stack.get_count();
			instruction_ptr->ret_addr = varity_number * 8;
			if(!function_ptr->variable_para_flag) {
				instruction_ptr->opdb_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);//确认再确认，此处和中间代码函数调用运算符时栈的申请的联动处理。
			} else {
				instruction_ptr->opdb_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
			}
			instruction_ptr->data = instruction_ptr->opdb_addr / PLATFORM_WORD_LEN;
			instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
			((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
			((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
			rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
			rvarity_ptr->config_complex_info(return_varity_ptr->get_complex_arg_count(), return_varity_ptr->get_complex_ptr());
			inc_varity_ref(rvarity_ptr);
			this->mid_varity_stack.push();
			this->call_func_info.function_depth--;
			if(this->call_func_info.function_depth) {
				code_stack_ptr->push();
				instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
				instruction_ptr->ret_operator = SYS_STACK_STEP;
				instruction_ptr->opda_addr = - make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4);
			}
		}
		break;
	case OPT_TYPE_CONVERT:
		varity_number = this->mid_varity_stack.get_count();
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->config_complex_info(((node_attribute_t*)opt_node_ptr->value)->count, (PLATFORM_WORD*)((node_attribute_t*)opt_node_ptr->value)->value.ptr_value);
		this->mid_varity_stack.push();
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = instruction_ptr->opda_addr = varity_number * 8;
		instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = rvarity_ptr->get_type();
		instruction_ptr->ret_operator = OPT_ASSIGN;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];		
		break;
	case OPT_EDGE:
		error("Extra ;\n");
		RETURN(ERROR_SEMICOLON);
	case OPT_SIZEOF:
		--this->sentence_analysis_data_struct.sizeof_depth;
		while(this->cur_mid_code_stack_ptr->get_count() > this->sentence_analysis_data_struct.sizeof_code_count[this->sentence_analysis_data_struct.sizeof_depth]) {
			this->cur_mid_code_stack_ptr->pop();
		}
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_CONST_VALUE;
		((node_attribute_t*)opt_node_ptr->value)->value_type = U_INT;
		if(((node_attribute_t*)opt_node_ptr->right->value)->node_type == TOKEN_NAME) {
			if(((node_attribute_t*)opt_node_ptr->right->value)->value.ptr_value[0] == TMP_VAIRTY_PREFIX || ((node_attribute_t*)opt_node_ptr->right->value)->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {
				varity_number = ((node_attribute_t*)opt_node_ptr->right->value)->value.ptr_value[1];
				rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
			} else
				rvarity_ptr = (varity_info*)this->varity_declare->find(((node_attribute_t*)opt_node_ptr->right->value)->value.ptr_value, PRODUCED_DECLARE);
			if(rvarity_ptr) {
				((node_attribute_t*)opt_node_ptr->value)->value.int_value = get_varity_size(0, rvarity_ptr->get_complex_ptr(), rvarity_ptr->get_complex_arg_count());
			} else {
				error("Varity not exist.\n");
				RETURN(ERROR_VARITY_NONEXIST);
			}
		} else if(((node_attribute_t*)opt_node_ptr->right->value)->node_type == TOKEN_CONST_VALUE) {

		} else if(((node_attribute_t*)opt_node_ptr->right->value)->node_type == TOKEN_OPERATOR && ((node_attribute_t*)opt_node_ptr->right->value)->data == OPT_TYPE_CONVERT) {
			((node_attribute_t*)opt_node_ptr->value)->value.int_value = get_varity_size(0, (PLATFORM_WORD*)((node_attribute_t*)opt_node_ptr->right->value)->value.ptr_value, ((node_attribute_t*)opt_node_ptr->right->value)->count);
		} else {
			error("Wrong use of sizeof.\n");
			RETURN(ERROR_SIZEOF);
		}
		RETURN(ERROR_NO);
	default:
		error("what??\n");
		break;
	}
	code_stack_ptr->push();
	RETURN(ERROR_NO);
}
#undef RETURN

int c_interpreter::operator_mid_handle(stack *code_stack_ptr, node *opt_node_ptr)
{
	register mid_code *instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
	node_attribute_t *node_attribute = ((node_attribute_t*)opt_node_ptr->value);//避免滥用&，误修改了变量。
	switch(node_attribute->data) {
		varity_info *varity_ptr;
	case OPT_TERNARY_Q:
		this->generate_expression_value(code_stack_ptr, (node_attribute_t*)opt_node_ptr->left->value);
		this->mid_varity_stack.pop();
		varity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(this->mid_varity_stack.get_count());
		dec_varity_ref(varity_ptr, true);
		((node_attribute_t*)opt_node_ptr->left->value)->node_type = TOKEN_CONST_VALUE;
		instruction_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		instruction_ptr->ret_operator = CTL_BRANCH_FALSE;
		this->sentence_analysis_data_struct.short_calc_stack[this->sentence_analysis_data_struct.short_depth++] = (PLATFORM_WORD)instruction_ptr;
		code_stack_ptr->push();
		break;
	case OPT_CALL_FUNC:
		this->call_func_info.function_ptr[this->call_func_info.function_depth] = this->function_declare->find(((node_attribute_t*)opt_node_ptr->left->value)->value.ptr_value);
		this->call_func_info.arg_count[this->call_func_info.function_depth] = this->call_func_info.function_ptr[this->call_func_info.function_depth]->arg_list->get_count();
		this->call_func_info.cur_arg_number[this->call_func_info.function_depth] = 0;
		this->call_func_info.offset[this->call_func_info.function_depth] = 0;
		if(this->call_func_info.function_depth) {
			instruction_ptr->ret_operator = SYS_STACK_STEP;
			instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4);
			code_stack_ptr->push();
		}
		this->call_func_info.function_depth++;
		break;
	case OPT_FUNC_COMMA:
		if(((node_attribute_t*)opt_node_ptr->left->value)->node_type == TOKEN_NAME ||  ((node_attribute_t*)opt_node_ptr->left->value)->node_type == TOKEN_CONST_VALUE || ((node_attribute_t*)opt_node_ptr->left->value)->node_type == TOKEN_STRING) {
			stack *arg_list_ptr = (stack*)this->call_func_info.function_ptr[this->call_func_info.function_depth - 1]->arg_list;
			node_attribute = (node_attribute_t*)opt_node_ptr->left->value;
			if(node_attribute->node_type == TOKEN_CONST_VALUE) {
				instruction_ptr->opdb_operand_type = OPERAND_CONST;
				instruction_ptr->opdb_varity_type = node_attribute->value_type;
				kmemcpy(&instruction_ptr->opdb_addr, &node_attribute->value, 8);
			} else if(node_attribute->node_type == TOKEN_STRING) {
				instruction_ptr->opdb_operand_type = OPERAND_G_VARITY;
				instruction_ptr->opdb_varity_type = ARRAY;
				instruction_ptr->opdb_addr = (int)node_attribute->value.ptr_value;
			} else if(node_attribute->node_type == TOKEN_NAME) {
				if(node_attribute->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {
					instruction_ptr->opdb_operand_type = OPERAND_T_VARITY;
					instruction_ptr->opdb_addr = 8 * node_attribute->value.ptr_value[1];
					instruction_ptr->opdb_varity_type = ((varity_info*)this->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]))->get_type();
				} else {
					int varity_scope;
					varity_ptr = this->varity_declare->vfind(node_attribute->value.ptr_value, varity_scope);
					if(!varity_ptr) {
						error("Varity not exist.\n");
						return ERROR_VARITY_NONEXIST;
					}
					if(varity_scope == VARITY_SCOPE_GLOBAL)
						instruction_ptr->opdb_operand_type = OPERAND_G_VARITY;
					else
						instruction_ptr->opdb_operand_type = OPERAND_L_VARITY;
					instruction_ptr->opdb_addr = (int)varity_ptr->get_content_ptr();
					instruction_ptr->opdb_varity_type = varity_ptr->get_type();
				}
			}
			varity_info *arg_varity_ptr;
			instruction_ptr->ret_operator = OPT_PASS_PARA;
			instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type = OPERAND_L_S_VARITY;
#if CALL_CONVENTION
			instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
#endif
			if(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] >= this->call_func_info.arg_count[this->call_func_info.function_depth - 1] - 1) {
				if(!this->call_func_info.function_ptr[this->call_func_info.function_depth - 1]->variable_para_flag) {
					error("Too many parameters\n");
					return ERROR_FUNC_ARGS;
				} else {
					if(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] == this->call_func_info.arg_count[this->call_func_info.function_depth - 1]) {
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
					}
					if(instruction_ptr->opdb_varity_type <= U_LONG && instruction_ptr->opdb_varity_type >= CHAR) {
						instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = INT;
#if !CALL_CONVENTION
						instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4);
						this->call_func_info.offset[this->call_func_info.function_depth - 1] += 4;
#else
						this->call_func_info.offset[this->call_func_info.function_depth - 1] += 4;
#endif
					} else if(instruction_ptr->opdb_varity_type == FLOAT || instruction_ptr->opdb_varity_type == DOUBLE) {
						instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = DOUBLE;
#if !CALL_CONVENTION
						instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8);
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + 8;
#else
						this->call_func_info.offset[this->call_func_info.function_depth - 1] += 8;
#endif
					} else if(instruction_ptr->opdb_varity_type == PTR || instruction_ptr->opdb_varity_type == ARRAY) {
						instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = PLATFORM_TYPE;
#if !CALL_CONVENTION
						instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) + PLATFORM_WORD_LEN;
#else
						this->call_func_info.offset[this->call_func_info.function_depth - 1] += PLATFORM_WORD_LEN;
#endif
					} else {
						instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = LONG_LONG;
#if !CALL_CONVENTION
						instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8);
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + 8;
#else
						this->call_func_info.offset[this->call_func_info.function_depth - 1] += 8;
#endif
					}
				}
			} else {
				arg_varity_ptr = (varity_info*)(arg_list_ptr->visit_element_by_index(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1]));
#if !CALL_CONVENTION
				switch(instruction_ptr->opdb_varity_type) {
				case DOUBLE:
				case LONG_LONG:
				case U_LONG_LONG:
					instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8);
					this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 8) + arg_varity_ptr->get_size();
					break;
				case PTR:
				case ARRAY:
					instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
					this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN) + arg_varity_ptr->get_size();
					break;
				default:
					instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4);
					this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], 4) + arg_varity_ptr->get_size();
					break;
				}
#else
				this->call_func_info.offset[this->call_func_info.function_depth - 1] += arg_varity_ptr->get_size();
#endif
				instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = arg_varity_ptr->get_type();
			}
			this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1]++;
			code_stack_ptr->push();
		}
		break;
	case OPT_AND://TODO:和OR合并
		this->generate_expression_value(code_stack_ptr, (node_attribute_t*)opt_node_ptr->left->value);
		instruction_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		instruction_ptr->ret_operator = CTL_BRANCH_TRUE;
		instruction_ptr->opda_addr = 3;
		code_stack_ptr->push();
		instruction_ptr++;
		instruction_ptr->ret_operator = OPT_ASSIGN;
		instruction_ptr->ret_varity_type = INT;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		instruction_ptr->opdb_addr = 0;
		instruction_ptr->opdb_varity_type = INT;
		instruction_ptr->opdb_operand_type = OPERAND_CONST;
		instruction_ptr->opda_varity_type = INT;
		instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
		code_stack_ptr->push();
		instruction_ptr++;
		instruction_ptr->ret_operator = CTL_BRANCH;
		this->sentence_analysis_data_struct.short_calc_stack[this->sentence_analysis_data_struct.short_depth++] = (PLATFORM_WORD)this->cur_mid_code_stack_ptr->get_current_ptr();
		code_stack_ptr->push();
		break;
	case OPT_OR:
		this->generate_expression_value(code_stack_ptr, (node_attribute_t*)opt_node_ptr->left->value);
		instruction_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		instruction_ptr->ret_operator = CTL_BRANCH_FALSE;
		instruction_ptr->opda_addr = 3;
		code_stack_ptr->push();
		instruction_ptr++;
		instruction_ptr->ret_operator = OPT_ASSIGN;
		instruction_ptr->ret_varity_type = INT;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		instruction_ptr->opdb_addr = 1;
		instruction_ptr->opdb_varity_type = INT;
		instruction_ptr->opdb_operand_type = OPERAND_CONST;
		instruction_ptr->opda_varity_type = INT;
		instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
		code_stack_ptr->push();
		instruction_ptr++;
		instruction_ptr->ret_operator = CTL_BRANCH;
		this->sentence_analysis_data_struct.short_calc_stack[this->sentence_analysis_data_struct.short_depth++] = (PLATFORM_WORD)this->cur_mid_code_stack_ptr->get_current_ptr();
		code_stack_ptr->push();
		break;
	}
	return ERROR_NO;
}

int c_interpreter::operator_pre_handle(stack *code_stack_ptr, node *opt_node_ptr)
{
	register mid_code *instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
	node_attribute_t *node_attribute = ((node_attribute_t*)opt_node_ptr->value);
	switch(node_attribute->data) {
	case OPT_TERNARY_C:
		if(((node_attribute_t*)opt_node_ptr->left->value)->data != OPT_TERNARY_Q) {
			error("? && : unmatch.\n");
			return ERROR_TERNARY_UNMATCH;
		}
	case OPT_SIZEOF:
		this->sentence_analysis_data_struct.sizeof_code_count[this->sentence_analysis_data_struct.sizeof_depth++] = this->cur_mid_code_stack_ptr->get_count();
		break;
	}
	return ERROR_NO;
}

int c_interpreter::tree_to_code(node *tree, stack *code_stack)
{
	register int ret;
	if(((node_attribute_t*)tree->value)->node_type == TOKEN_OPERATOR) {	//需要先序处理的运算符：TERNERY_C
		ret = this->operator_pre_handle(code_stack, tree);
		if(ret)
			return ret;
	}
	if(tree->left && ((node_attribute_t*)tree->left->value)->node_type == TOKEN_OPERATOR) {
		ret = this->tree_to_code(tree->left, code_stack);
		if(ret)
			return ret;
	}
	if(((node_attribute_t*)tree->value)->node_type == TOKEN_OPERATOR) {	//需要中序处理的几个运算符：CALL_FUNC，&&，||，FUNC_COMMA等
		ret = this->operator_mid_handle(code_stack, tree);
		if(ret)
			return ret;
	}
	if(tree->right && ((node_attribute_t*)tree->right->value)->node_type == TOKEN_OPERATOR) {
		ret = this->tree_to_code(tree->right, code_stack);
		if(ret)
			return ret;
	}
	if(((node_attribute_t*)tree->value)->node_type == TOKEN_OPERATOR) {
		ret = this->operator_post_handle(code_stack, tree);
		if(ret)
			return ret;
	}
	return 0;
}

int c_interpreter::test(char *str, uint len)
{
	int token_len;
	node_attribute_t node_attribute;
	while(len > 0) {
		token_len = get_token(str, &node_attribute);
		len -= token_len;
		str += token_len;
	}
	return 0;
}

int c_interpreter::pre_treat(uint len)
{
	int spacenum = 0;
	int rptr = 0, wptr = 0, first_word = 1, space_flag = 0;
	int string_flag = 0;
	char bracket_stack[32], bracket_depth = 0;
	char nowchar;
	while(nowchar = sentence_buf[rptr]) {
		if(IsSpace(nowchar)) {
			if(!first_word) {
				if(!space_flag)
					sentence_buf[wptr++] = ' ';
				space_flag = 1;
			}
		} else {
			switch(nowchar) {
			case '(':
			case '[':
				if(!string_flag) {
					bracket_stack[bracket_depth++] = nowchar;
				}
				break;
			case ')':
				if(!string_flag) {
					if(bracket_depth-- > 0 && bracket_stack[bracket_depth] == '(') {
					} else {
						error("Bracket unmatch.\n");
						return ERROR_BRACKET_UNMATCH;
					}
				}
				break;
			case ']':
				if(!string_flag) {
					if(bracket_depth-- > 0 && bracket_stack[bracket_depth] == '[') {
					} else {
						error("Bracket unmatch.\n");
						return ERROR_BRACKET_UNMATCH;
					}
				}
				break;
			case '"':
				if(!string_flag) {
					string_flag = '"';
				} else if(string_flag == '"') {
					if(sentence_buf[rptr - 1] != '\\') {
						string_flag = 0;
					}
				}
				break;
			case '\'':
				if(!string_flag) {
					string_flag = '\'';
				} else if(string_flag == '\'') {
					if(sentence_buf[rptr - 1] != '\\') {
						string_flag = 0;
					}
				}
				break;
			case '{':
			case '}':
				if(!bracket_depth && !string_flag) {
					if(rptr) {
						this->row_pretreat_fifo.write(sentence_buf + rptr, len - rptr);
						sentence_buf[rptr] = 0;
					} else {
						this->row_pretreat_fifo.write(sentence_buf + rptr + 1, len - rptr - 1);
						sentence_buf[rptr + 1] = 0;
					}
				}
				break;
			case ';':
				if(!bracket_depth && !string_flag) {
					this->row_pretreat_fifo.write(sentence_buf + rptr + 1, len - rptr - 1);
					sentence_buf[rptr + 1] = 0;
				}
			}

			space_flag = 0;
			first_word = 0;
			sentence_buf[wptr++] = sentence_buf[rptr];
		}
		rptr++;
	}
	if(bracket_depth) {
		error("Bracket unmatch.\n");
		return ERROR_BRACKET_UNMATCH;
	}
	sentence_buf[wptr] = 0;
	return wptr;
}

int c_interpreter::run_interpreter(void)
{
	int ret;
	this->init(&stdio);
	this->generate_compile_func();
	while(1) {
		int len;
		len = this->row_pretreat_fifo.readline(sentence_buf);
		if(len > 0) {

		} else {
			kprintf(">>> ");
			len = tty_used->readline(sentence_buf);
		}
		len = pre_treat(len);
		if(len < 0)
			continue;
		ret = this->sentence_analysis(sentence_buf, len);
		if(ret == OK_FUNC_RETURN)
			return ret;
	}
	return ERROR_NO;
}

int c_interpreter::init(terminal* tty_used)
{
	if(!c_interpreter::language_elment_space.init_done) {
		for(int i=0; i<sizeof(basic_type_info)/sizeof(basic_type_info[0]); i++) {
			c_interpreter::varity_type_stack.arg_count[i] = 2;
			c_interpreter::varity_type_stack.type_info_addr[i] = basic_type_info[i];
		}
		c_interpreter::varity_type_stack.arg_count[STRUCT] = 3;
		c_interpreter::varity_type_stack.count = sizeof(basic_type_info) / sizeof(basic_type_info[0]);
		c_interpreter::language_elment_space.l_varity_list.init(sizeof(varity_info), c_interpreter::language_elment_space.l_varity_node, MAX_L_VARITY_NODE);
		c_interpreter::language_elment_space.g_varity_list.init(sizeof(varity_info), c_interpreter::language_elment_space.g_varity_node, MAX_G_VARITY_NODE);
		c_interpreter::language_elment_space.c_varity.init(&c_interpreter::language_elment_space.g_varity_list, &c_interpreter::language_elment_space.l_varity_list);
		c_interpreter::language_elment_space.function_list.init(sizeof(function_info), c_interpreter::language_elment_space.function_node, MAX_FUNCTION_NODE);
		c_interpreter::language_elment_space.c_function.init(&c_interpreter::language_elment_space.function_list);
		c_interpreter::language_elment_space.struct_list.init(sizeof(struct_info), c_interpreter::language_elment_space.struct_node, MAX_STRUCT_NODE);
		c_interpreter::language_elment_space.c_struct.init(&c_interpreter::language_elment_space.struct_list);
		this->varity_declare = &c_interpreter::language_elment_space.c_varity;
		this->nonseq_info = &c_interpreter::language_elment_space.nonseq_info_s;
		this->function_declare = &c_interpreter::language_elment_space.c_function;
		this->struct_declare = &c_interpreter::language_elment_space.c_struct;
		this->nonseq_info->nonseq_begin_stack_ptr = 0;
		this->sentence_analysis_data_struct.short_depth = 0;
		this->sentence_analysis_data_struct.sizeof_depth = 0;
		this->sentence_analysis_data_struct.label_count = 0;
		for(int i=0; i<MAX_A_VARITY_NODE; i++) {
			link_varity_name[i][0] = LINK_VARITY_PREFIX;
			tmp_varity_name[i][0] = TMP_VAIRTY_PREFIX;
			tmp_varity_name[i][1] = link_varity_name[i][1] = i;
			tmp_varity_name[i][2] = link_varity_name[i][2] = 0;
		}
		c_interpreter::language_elment_space.init_done = 1;
	}
	this->tty_used = tty_used;
	this->row_pretreat_fifo.set_base(this->pretreat_buf);
	this->row_pretreat_fifo.set_length(sizeof(this->pretreat_buf));
	this->row_pretreat_fifo.set_element_size(1);
	this->non_seq_code_fifo.set_base(this->non_seq_tmp_buf);
	this->non_seq_code_fifo.set_length(sizeof(this->non_seq_tmp_buf));
	this->non_seq_code_fifo.set_element_size(1);
	this->call_func_info.function_depth = 0;
	this->varity_global_flag = VARITY_SCOPE_GLOBAL;
	///////////
	handle_init();
	this->break_flag = 0;
	this->stack_pointer = this->simulation_stack;
	this->tmp_varity_stack_pointer = this->tmp_varity_stack;
	this->mid_code_stack.init(sizeof(mid_code), MAX_MID_CODE_COUNT);
	this->mid_varity_stack.init(sizeof(varity_info), this->tmp_varity_stack_pointer, MAX_A_VARITY_NODE);//TODO: 设置node最大count
	this->cur_mid_code_stack_ptr = &this->mid_code_stack;
	this->exec_flag = true;
	return 0;
	///////////
}

void nonseq_info_struct::reset(void)
{
	kmemset(this, 0, sizeof(nonseq_info_struct));
}

int c_interpreter::save_sentence(char* str, uint len)
{
	nonseq_info->row_info_node[nonseq_info->row_num].row_ptr = &non_seq_tmp_buf[non_seq_code_fifo.wptr];
	nonseq_info->row_info_node[nonseq_info->row_num].row_len = len;
	non_seq_code_fifo.write(str, len);
	non_seq_code_fifo.write("\0", 1);
	return 0;
}

#define GET_BRACE_FLAG(x)	(((x) & 128) >> 7)
#define GET_BRACE_DATA(x)	((x) & 127)
#define SET_BRACE(x,y) (((x) << 7) | (y))
int c_interpreter::struct_end(int struct_end_flag, bool &exec_flag_bak, register bool try_flag)
{
	int ret = 0;
	int depth_bak = this->nonseq_info->non_seq_struct_depth;
	while(this->nonseq_info->non_seq_struct_depth > 0 
		&& ((GET_BRACE_DATA(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1]) == nonseq_info->brace_depth + 1 && GET_BRACE_FLAG(nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1]))
		|| (GET_BRACE_DATA(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1]) == nonseq_info->brace_depth && !GET_BRACE_FLAG(nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1])))) {
		if(!try_flag) {
			this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1] = 0;
			this->varity_declare->local_varity_stack->dedeep();
		}
		if(nonseq_info->non_seq_struct_depth > 0 && nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] == NONSEQ_KEY_IF && !(struct_end_flag & 2)) {
			if(!try_flag)
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] = NONSEQ_KEY_WAIT_ELSE;
			--nonseq_info->non_seq_struct_depth;
			break;
		}
		--nonseq_info->non_seq_struct_depth;
		if(nonseq_info->non_seq_struct_depth >= 0 && !try_flag)
			nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = 0;
	}
	if(struct_end_flag & 1) {
		nonseq_info->row_info_node[nonseq_info->row_num].non_seq_info = 1;
		nonseq_info->row_info_node[nonseq_info->row_num].non_seq_depth = nonseq_info->non_seq_struct_depth + 1;
	}
	if(nonseq_info->non_seq_struct_depth == 0) {
		if(nonseq_info->non_seq_type_stack[0] == NONSEQ_KEY_WAIT_ELSE) {
			ret = OK_NONSEQ_INPUTING;
		} else if(nonseq_info->non_seq_type_stack[0] == NONSEQ_KEY_WAIT_WHILE) {
			ret = OK_NONSEQ_INPUTING;
		} else {
			if(!try_flag) {
				nonseq_info->non_seq_exec = 1;
				this->exec_flag = exec_flag_bak;
			}
			ret = OK_NONSEQ_FINISH;
		}
	}
	if(try_flag)
		this->nonseq_info->non_seq_struct_depth = depth_bak;
	return ret;
}

#define RETURN(x) \
	return x
int c_interpreter::non_seq_struct_analysis(char* str, uint len)
{
	static bool exec_flag_bak;
	int struct_end_flag = 0;
	int ret = ERROR_NO;
	int current_brace_level = 0;
	nonseq_info->last_non_seq_check_ret = nonseq_info->non_seq_check_ret;
	nonseq_info->non_seq_check_ret = non_seq_struct_check(str);
	current_brace_level = nonseq_info->brace_depth;
	if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_WAIT_ELSE) {
		if(nonseq_info->brace_depth == 0) {
			if(len != 0 && nonseq_info->non_seq_check_ret != NONSEQ_KEY_ELSE) {
				error("if is unmatch with else or blank\n");
				return ERROR_NONSEQ_GRAMMER;
			} else if(len == 0) {
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_IF;
				struct_end_flag = 2;
			} else { //nonseq_info->non_seq_check_ret == NONSEQ_KEY_ELSE
				nonseq_mid_gen_mid_code(str, len);//这句不会出错，不加返回值判断了
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 3;
			}
		} else {
			if(nonseq_info->non_seq_check_ret != NONSEQ_KEY_ELSE && len != 0) {//TODO:类型于函数中的if，对函数中的if处理可照此执行
				struct_end_flag = 2;
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_IF;
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth = nonseq_info->non_seq_struct_depth + 1;
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 1;
				nonseq_end_gen_mid_code(nonseq_info->row_num - 1, 0, 0);
				struct_end(struct_end_flag, exec_flag_bak, 0);
				struct_end_flag = 0;
			} else if(nonseq_info->non_seq_check_ret == NONSEQ_KEY_ELSE) {
				nonseq_mid_gen_mid_code(str, len);
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 3;
			}
		}
	} else if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] == NONSEQ_KEY_WAIT_WHILE) {
		if(nonseq_info->non_seq_check_ret != NONSEQ_KEY_WHILE && len != 0) {
			error("do is unmatch with while\n");
			return ERROR_NONSEQ_GRAMMER;
 		} else if(nonseq_info->non_seq_check_ret == NONSEQ_KEY_WHILE) {
			struct_end_flag = 1;
			goto struct_end_check;
		}
	}

	if(str[0] == '{' && nonseq_info->non_seq_struct_depth) {
		nonseq_info->brace_depth++;
		if(nonseq_info->last_non_seq_check_ret) {
			this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1] = SET_BRACE(1, nonseq_info->brace_depth);
		} else {
			this->varity_declare->local_varity_stack->endeep();
		}
	} else if(str[0] == '}') {
		if(nonseq_info->brace_depth > 0) {
			if(nonseq_info->non_seq_struct_depth > 0 && GET_BRACE_DATA(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1]) == nonseq_info->brace_depth) {
				if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] != NONSEQ_KEY_DO) {
					if(!struct_end_flag)
						struct_end_flag = 1;
				} else
					nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] = NONSEQ_KEY_WAIT_WHILE;
			} else {
				this->varity_declare->local_varity_stack->dedeep();
			}
			nonseq_info->brace_depth--;
		} else {
			error("there is no { to match\n");
			return ERROR_NONSEQ_GRAMMER;
		}
	}

	if(nonseq_info->non_seq_check_ret) {
		if(nonseq_info->row_num == 0) {
			exec_flag_bak = this->exec_flag;
			this->exec_flag = 0;
		}
		this->varity_declare->local_varity_stack->endeep();
		ret = nonseq_start_gen_mid_code(str, len, nonseq_info->non_seq_check_ret);
		if(ret) {
			error("Nonseq struct error.\n");
			this->varity_declare->local_varity_stack->dedeep();
			if(nonseq_info->row_num == 0)
				this->exec_flag = exec_flag_bak;
			return ret;
		}
		this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] = SET_BRACE(0, nonseq_info->brace_depth);
		nonseq_info->row_info_node[nonseq_info->row_num].non_seq_info = 0;
		if(nonseq_info->non_seq_check_ret == NONSEQ_KEY_ELSE) {
			nonseq_info->row_info_node[nonseq_info->row_num].non_seq_info = 2;
			if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] != NONSEQ_KEY_WAIT_ELSE) {//|| nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] != nonseq_info->brace_depth
				error("else is unmatch with if\n");
				return ERROR_NONSEQ_GRAMMER;
			}
		}
		nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = nonseq_info->non_seq_check_ret;
		nonseq_info->non_seq_struct_depth++;
		nonseq_info->row_info_node[nonseq_info->row_num].non_seq_depth = nonseq_info->non_seq_struct_depth;
		nonseq_info->row_info_node[nonseq_info->row_num].nonseq_type = nonseq_info->non_seq_check_ret;
		ret = OK_NONSEQ_DEFINE;
	}
	if(str[len - 1] == ';') {
		if(nonseq_info->non_seq_struct_depth) {
			ret = this->sentence_exec(str, len, true);
			if(ret) {
				nonseq_info->non_seq_check_ret = nonseq_info->last_non_seq_check_ret;//恢复上一句结果，否则重输正确语句无法结束非顺序结构
				return ret;
			}
		}
		if(nonseq_info->last_non_seq_check_ret && nonseq_info->non_seq_struct_depth) {// && nonseq_info->brace_depth == 0
			if(nonseq_info->last_non_seq_check_ret != NONSEQ_KEY_DO)
				struct_end_flag = 1;
			else
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] = NONSEQ_KEY_WAIT_WHILE;
		}
	}
struct_end_check:
	if(struct_end_flag) {
		ret = struct_end(struct_end_flag, exec_flag_bak, 1);
		nonseq_end_gen_mid_code(nonseq_info->row_num, str, len);
		ret = struct_end(struct_end_flag, exec_flag_bak, 0);
	}
	if(len && (nonseq_info->non_seq_struct_depth || nonseq_info->row_info_node[nonseq_info->row_num].non_seq_depth)) {// && (nonseq_info->non_seq_check_ret || nonseq_info->non_seq_struct_depth)) {
		this->save_sentence(str, len);
		nonseq_info->row_num++;
	}
	if(!ret) {
		if(len == 0)
			ret = OK_NONSEQ_INPUTING;
		if(nonseq_info->non_seq_struct_depth)
			ret = OK_NONSEQ_INPUTING;
	}
	return ret;
}
#undef RETURN

int c_interpreter::generate_compile_func(void)
{
	static stack memcpy_stack;
	this->generate_arg_list("void*,void*,void*,unsigned int;", 4, memcpy_stack);
	this->function_declare->add_compile_func("memcpy", (void*)kmemcpy, &memcpy_stack, 0);
	static stack memset_stack;
	this->generate_arg_list("void*,void*,int,unsigned int;", 4, memset_stack);
	this->function_declare->add_compile_func("memset", (void*)kmemset, &memset_stack, 0);
	static stack printf_stack;
	this->generate_arg_list("int,char*;", 2, printf_stack);
	this->function_declare->add_compile_func("printf", (void*)kprintf, &printf_stack, 1);
	static stack sprintf_stack;
	this->generate_arg_list("int,char*,char*;", 3, sprintf_stack);
	this->function_declare->add_compile_func("sprintf", (void*)ksprintf, &sprintf_stack, 1);
	return ERROR_NO;
}

int c_interpreter::generate_arg_list(char *str, int count, stack &arg_list_ptr)//没有容错，不开放给终端输入，仅用于链接标准库函数
{
	int len = kstrlen(str);
	void *arg_stack = vmalloc(sizeof(varity_info) * count);
	arg_list_ptr.init(sizeof(varity_info), arg_stack, count);
	varity_info *varity_ptr = (varity_info*)arg_list_ptr.get_base_addr();
	node_attribute_t node;
	int token_len;
	int type;
	char ptr_flag = 0;
	while(len > 0) {
		token_len = get_token(str, &node);
		if(node.node_type == TOKEN_KEYWORD_TYPE) {
			type = node.value.int_value;
		} else if(node.node_type == TOKEN_OPERATOR ) {
			if(node.data == OPT_MUL) {
				ptr_flag = 1;
			} else {
				varity_info::init_varity(varity_ptr, 0, type, 4, 1 + ptr_flag, basic_type_info[type]);
				varity_ptr++;
				arg_list_ptr.push();
				ptr_flag = 0;
			}
		}
		str += token_len;
		len -= token_len;
	}
	return ERROR_NO;
}

#define ARG_RETURN(x) this->function_flag_set.function_flag = 0; \
	this->function_flag_set.function_begin_flag = 0; \
	destroy_varity_stack(arg_stack); \
	vfree(arg_stack->get_base_addr()); \
	vfree(arg_stack); \
	this->varity_declare->destroy_local_varity(); \
	this->cur_mid_code_stack_ptr = &this->mid_code_stack; \
	this->exec_flag = true; \
	this->varity_global_flag = VARITY_SCOPE_GLOBAL; \
	return x
int c_interpreter::function_analysis(char* str, uint len)
{//TODO:非调试使能删除函数源代码
	int ret_function_define;
	if(this->function_flag_set.function_flag) {
		this->function_declare->save_sentence(str, len);
		if(this->function_flag_set.function_begin_flag) {
			if(str[0] != '{' || len != 1) {
				error("Please enter {\n");
				return ERROR_FUNC_ERROR;
			}
			this->function_flag_set.function_begin_flag = 0;
		}
		if(str[0] == '{') {
			this->function_flag_set.brace_depth++;
		} else if(str[0] == '}') {
			this->function_flag_set.brace_depth--;
			if(!this->function_flag_set.brace_depth) {
				function_info *current_function_ptr = this->function_declare->get_current_node();
				mid_code *mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr(), *code_end_ptr = mid_code_ptr;
				while(--mid_code_ptr >= (mid_code*)current_function_ptr->mid_code_stack.get_base_addr()) {
					if(mid_code_ptr->ret_operator == CTL_RETURN)
						mid_code_ptr->opda_addr = code_end_ptr - mid_code_ptr;
					else if(mid_code_ptr->ret_operator == CTL_GOTO) {
						int i;
						for(i=0; i<this->sentence_analysis_data_struct.label_count; i++) {
							if(!kstrcmp(this->sentence_analysis_data_struct.label_name[i], (char*)&mid_code_ptr->ret_addr)) {
								mid_code_ptr->opda_addr = (mid_code*)this->sentence_analysis_data_struct.label_addr[i] - mid_code_ptr;
								break;
							}
						}
						if(i == this->sentence_analysis_data_struct.label_count) {
							error("No label called \"%s\"\n", (char*)&mid_code_ptr->ret_addr);
							current_function_ptr->reset();
							vfree(current_function_ptr->arg_list->get_base_addr());
							vfree(current_function_ptr->arg_list);
							this->varity_declare->destroy_local_varity();
							return ERROR_GOTO_LABEL;
						}
					}
				}
				this->function_flag_set.function_flag = 0;
				this->cur_mid_code_stack_ptr = &this->mid_code_stack;
				current_function_ptr->stack_frame_size = this->varity_declare->local_varity_stack->offset;
				current_function_ptr->size_adapt();
				this->exec_flag = true;
				this->varity_global_flag = VARITY_SCOPE_GLOBAL;
				this->varity_declare->destroy_local_varity();
				this->sentence_analysis_data_struct.label_count = 0;//TODO:清理工作在失败时做彻底
				return OK_FUNC_FINISH;
			}
		}
		return OK_FUNC_INPUTING;
	}
	int basic_type_len;
	struct_info *struct_info_ptr;
	ret_function_define = basic_type_check(str, basic_type_len, struct_info_ptr);
	//ret_function_define = optcmp(str);
	if(ret_function_define >= 0) {
		char function_name[32];
		char *str_bak = str;
		int v_len = len;
		node_attribute_t node_ptr;
		int token_len;
		int function_declare_flag = 0;
		while(v_len > 0) {
			token_len = get_token(str_bak, &node_ptr);
			if(node_ptr.node_type == TOKEN_NAME) {
				kstrcpy(function_name, node_ptr.value.ptr_value);
				get_token(str_bak + token_len, &node_ptr);
				if(node_ptr.node_type == TOKEN_OPERATOR && (node_ptr.data == OPT_L_SMALL_BRACKET || node_ptr.data == OPT_TYPE_CONVERT) || node_ptr.node_type == TOKEN_ARG_LIST) {
					str = str_bak;
					function_declare_flag = 1;
					break;
				}
			}
			v_len -= token_len;
			str_bak += token_len;
		}
		int l_bracket_pos = find_ch_with_bracket_level(str, '(', 0);
		int r_bracket_pos = find_ch_with_bracket_level(str, ')', 1);
		if(function_declare_flag) {
			if(!this->exec_flag) {
				error("Cannot define function here.\n");
				return ERROR_FUNC_DEF_POS;
			}
			int keylen = kstrlen(type_key[ret_function_define]);
			stack* arg_stack;
			this->function_flag_set.function_flag = 1;
			this->function_flag_set.function_begin_flag = 1;
			varity_info* arg_node_ptr = (varity_info*)vmalloc(sizeof(varity_info) * MAX_FUNCTION_ARGC);
			arg_stack = (stack*)vmalloc(sizeof(stack));
			arg_stack->init(sizeof(varity_info), arg_node_ptr, MAX_FUNCTION_ARGC);
			arg_node_ptr->arg_init("", ret_function_define, sizeof_type[ret_function_define], 0);//TODO:加上offset
			arg_stack->push(arg_node_ptr++);
			bool void_flag = false;
			int offset = 0;
			this->varity_global_flag = VARITY_SCOPE_LOCAL;
			for(int i=l_bracket_pos+1; i<r_bracket_pos;) {
				char varity_name[32];
				int type;
				struct_info *struct_info_ptr;
				PLATFORM_WORD *varity_complex_ptr;
				int type_len, complex_node_count;
				token_len = get_token(str + i, &node_ptr);
				if(token_len > r_bracket_pos - i) {
					break;
				}
				type = basic_type_check(str + i, type_len, struct_info_ptr);
				//int pos = key_match(str + i, r_bracket_pos-i+1, &type);
				if(type < 0) {     
					error("arg type error.\n");
					ARG_RETURN(ERROR_FUNC_ARG_LIST);
				}
				i += type_len;
				type_len = r_bracket_pos - i;
				complex_node_count = get_varity_type(str + i, type_len, varity_name, type, struct_info_ptr, varity_complex_ptr);
				i += type_len;
				if(type == VOID) {
					if(complex_node_count == 1 || (complex_node_count > 1 && varity_complex_ptr[complex_node_count] != PTR)) {
						if(arg_stack->get_count() > 1) {
							error("arg list error.\n");
							ARG_RETURN(ERROR_FUNC_ARG_LIST);
						}
					}
					void_flag = true;
				} else {
					if(void_flag) {
						error("arg cannot use void type.\n");
						ARG_RETURN(ERROR_FUNC_ARG_LIST);
					}
				}
				if(!void_flag) {
					arg_node_ptr->arg_init(varity_name, type, sizeof_type[type], (void*)make_align(offset, PLATFORM_WORD_LEN));
					arg_node_ptr->config_complex_info(complex_node_count, varity_complex_ptr);
					this->varity_declare->declare(VARITY_SCOPE_LOCAL, varity_name, type, sizeof_type[type], complex_node_count, varity_complex_ptr);
					arg_stack->push(arg_node_ptr++);
				} else {
					if(varity_name[0] != 0) {
						error("arg cannot use void type.\n");
						ARG_RETURN(ERROR_FUNC_ARG_LIST);
					}
				}
				offset = make_align(offset, PLATFORM_WORD_LEN) + sizeof_type[type];
			}
			this->function_declare->declare(function_name, arg_stack);
			this->cur_mid_code_stack_ptr = &this->function_declare->get_current_node()->mid_code_stack;
			this->exec_flag = false;
			this->function_declare->save_sentence(str, len);
			return OK_FUNC_DEFINE;
		}
	}
	return OK_FUNC_NOFUNC;
}
#undef ARG_RETURN

bool c_interpreter::is_operator_convert(char *str, char &type, int &opt_len, char &prio)
{
	switch(type) {
	case OPT_PLUS:
	case OPT_MINUS:
		if(this->sentence_analysis_data_struct.last_token.node_type == TOKEN_OPERATOR 
			&& this->sentence_analysis_data_struct.last_token.data != OPT_R_SMALL_BRACKET 
			&& this->sentence_analysis_data_struct.last_token.data != OPT_L_MINUS_MINUS 
			&& this->sentence_analysis_data_struct.last_token.data != OPT_L_PLUS_PLUS
			&& this->sentence_analysis_data_struct.last_token.data != OPT_R_PLUS_PLUS
			&& this->sentence_analysis_data_struct.last_token.data != OPT_R_MINUS_MINUS) {
			if(type == OPT_PLUS) {
				type = OPT_POSITIVE;
			} else {
				type = OPT_NEGATIVE;
			}
			prio = 2;
			return true;
		} 
		break;
	case OPT_PLUS_PLUS:
	case OPT_MINUS_MINUS:
		node_attribute_t next_node_attribute;
		int next_token_len;
		next_token_len = get_token(str + opt_len, &next_node_attribute);
		if(this->sentence_analysis_data_struct.last_token.node_type == TOKEN_NAME && next_node_attribute.node_type == TOKEN_NAME) {
			if(type == OPT_PLUS_PLUS) {
				type = OPT_PLUS;
			} else {
				type = OPT_MINUS;
			}
			opt_len = 1;
			prio = 4;
			return true;
		}
		break;
	case OPT_MUL:
	case OPT_BIT_AND:
		if(this->sentence_analysis_data_struct.last_token.node_type == TOKEN_OPERATOR 
			&& this->sentence_analysis_data_struct.last_token.data != OPT_R_SMALL_BRACKET 
			&& this->sentence_analysis_data_struct.last_token.data != OPT_L_MINUS_MINUS 
			&& this->sentence_analysis_data_struct.last_token.data != OPT_L_PLUS_PLUS
			&& this->sentence_analysis_data_struct.last_token.data != OPT_R_PLUS_PLUS
			&& this->sentence_analysis_data_struct.last_token.data != OPT_R_MINUS_MINUS) {
			if(type == OPT_MUL)
				type = OPT_PTR_CONTENT;
			else
				type = OPT_ADDRESS_OF;
			prio = 2;
			return true;
		}
		break;
	case OPT_L_MID_BRACKET:
		type = OPT_INDEX;
		return true;
	case OPT_R_MID_BRACKET:
		type = OPT_R_SMALL_BRACKET;
		return true;
	case OPT_L_SMALL_BRACKET:
		if(this->sentence_analysis_data_struct.last_token.node_type == TOKEN_NAME) {
			type = OPT_CALL_FUNC;
			return true;
		}
	default:
		break;
	}
	return false;
}

int c_interpreter::label_analysis(char *str, int len)
{
	int token_len;
	node_attribute_t node;
	token_len = get_token(str, &node);
	if(node.node_type == TOKEN_NAME) {
		char name[MAX_LABEL_NAME_LEN + 1];
		kstrcpy(name, node.value.ptr_value);
		str += token_len;
		token_len = get_token(str, &node);
		if(node.node_type == TOKEN_OPERATOR && node.data == OPT_TERNARY_C) {
			str += token_len;
			token_len = get_token(str, &node);
			if(node.node_type == TOKEN_ERROR) {
				if(this->cur_mid_code_stack_ptr == &this->mid_code_stack) {
					error("Label & \"goto\" cannot be used in main function.\n");
					return ERROR_GOTO_POSITION;
				}
				if(this->sentence_analysis_data_struct.label_count >= MAX_LABEL_COUNT) {
					error("Label count reach max.\n");
					return ERROR_GOTO_COUNT_MAX;
				}
				this->sentence_analysis_data_struct.label_addr[this->sentence_analysis_data_struct.label_count] = this->cur_mid_code_stack_ptr->get_current_ptr();
				kstrcpy(this->sentence_analysis_data_struct.label_name[this->sentence_analysis_data_struct.label_count++], name);
				return OK_LABEL_DEFINE;
			}
		}
	} else if(node.node_type == TOKEN_ERROR) {
		return ERROR_TOKEN;
	}
	return ERROR_NO;
}

int c_interpreter::generate_mid_code(char *str, int len, bool need_semicolon)//TODO:所有uint len统统改int，否则传个-1进来
{
	if(len == 0 || str[0] == '{' || str[0] == '}')return ERROR_NO;
	sentence_analysis_data_struct_t *analysis_data_struct_ptr = &this->sentence_analysis_data_struct;
	int token_len;
	int node_index = 0;
	node_attribute_t *node_attribute, *stack_top_node_ptr;
	this->sentence_analysis_data_struct.last_token.node_type = TOKEN_OPERATOR;
	this->sentence_analysis_data_struct.last_token.data = OPT_EDGE;
	mid_code *mid_code_ptr;
	if(!strmcmp(str, "return ", 7)) {
		int ret;
		if(this->cur_mid_code_stack_ptr == &this->mid_code_stack)
			return OK_FUNC_RETURN;
		ret = this->generate_mid_code(str + 7, len - 7, true);//下一句貌似重复了，所以注释掉。
		//this->generate_expression_value(this->cur_mid_code_stack_ptr, (node_attribute_t*)this->sentence_analysis_data_struct.tree_root->value);
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		mid_code_ptr->ret_operator = CTL_RETURN;
		this->cur_mid_code_stack_ptr->push();
		return ret;
	} else if(!strmcmp(str, "break;", 6)) {
		if(this->nonseq_info->row_num > 0) {
			int read_seq_depth = this->nonseq_info->non_seq_struct_depth > this->nonseq_info->row_info_node[this->nonseq_info->row_num].non_seq_depth ? this->nonseq_info->non_seq_struct_depth : this->nonseq_info->row_info_node[this->nonseq_info->row_num].non_seq_depth;
			for(int i=read_seq_depth; i>0; i--) {
				if(this->nonseq_info->non_seq_type_stack[i - 1] == NONSEQ_KEY_FOR || this->nonseq_info->non_seq_type_stack[i - 1] == NONSEQ_KEY_WHILE || this->nonseq_info->non_seq_type_stack[i - 1] == NONSEQ_KEY_DO) {
					mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
					mid_code_ptr->ret_operator = CTL_BREAK;
					this->cur_mid_code_stack_ptr->push();
					return ERROR_NO;
				}
			}
		}
		error("no nonseq struct for using break.\n");
		return ERROR_NONSEQ_CTL;
	} else if(!strmcmp(str, "continue;", 9)) {
		if(this->nonseq_info->row_num > 0) {
			int read_seq_depth = this->nonseq_info->non_seq_struct_depth > this->nonseq_info->row_info_node[this->nonseq_info->row_num].non_seq_depth ? this->nonseq_info->non_seq_struct_depth : this->nonseq_info->row_info_node[this->nonseq_info->row_num].non_seq_depth;
			for(int i=read_seq_depth; i>0; i--) {
				if(this->nonseq_info->non_seq_type_stack[i - 1] == NONSEQ_KEY_FOR || this->nonseq_info->non_seq_type_stack[i - 1] == NONSEQ_KEY_WHILE || this->nonseq_info->non_seq_type_stack[i - 1] == NONSEQ_KEY_DO) {
					mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
					mid_code_ptr->ret_operator = CTL_CONTINUE;
					this->cur_mid_code_stack_ptr->push();
					return ERROR_NO;
				}
			}
		}
		error("no nonseq struct for using continue.\n");
		return ERROR_NONSEQ_CTL;
	} else if(!strmcmp(str, "goto ", 5)) {
		token_len = get_token(str + 5, analysis_data_struct_ptr->node_attribute);
		if(analysis_data_struct_ptr->node_attribute->node_type != TOKEN_NAME) {
			error("There must be a label after goto.\n");
			return ERROR_GOTO_LABEL;
		}
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		mid_code_ptr->ret_operator = CTL_GOTO;
		kstrcpy((char*)&mid_code_ptr->ret_addr, analysis_data_struct_ptr->node_attribute->value.ptr_value);
		this->cur_mid_code_stack_ptr->push();
		return ERROR_NO;
	}
	while(len > 0) {
		node_attribute = &analysis_data_struct_ptr->node_attribute[node_index];
		analysis_data_struct_ptr->node_struct[node_index].value = node_attribute;
		token_len = get_token(str, node_attribute);
		if(len < token_len)
			break;
		if(node_attribute->node_type == TOKEN_ERROR) {
			analysis_data_struct_ptr->expression_final_stack.reset();
			analysis_data_struct_ptr->expression_tmp_stack.reset();
			error("Wrong symbol.\n");
			return ERROR_TOKEN;
		}
		if(node_attribute->node_type == TOKEN_OPERATOR) {
			while(1) {
				is_operator_convert(str, node_attribute->data, token_len, node_attribute->value_type);
				stack_top_node_ptr = (node_attribute_t*)analysis_data_struct_ptr->expression_tmp_stack.get_lastest_element()->value;
				if(!analysis_data_struct_ptr->expression_tmp_stack.get_count() 
					|| node_attribute->value_type < stack_top_node_ptr->value_type 
					|| node_attribute->data == OPT_L_SMALL_BRACKET 
					|| stack_top_node_ptr->data == OPT_L_SMALL_BRACKET
					|| (node_attribute->value_type == stack_top_node_ptr->value_type && (node_attribute->value_type == 2 || node_attribute->value_type == 14))) {
					if(node_attribute->data == OPT_R_SMALL_BRACKET) {
						analysis_data_struct_ptr->expression_tmp_stack.pop();
						break;
					} else {
						if(node_attribute->data == OPT_CALL_FUNC) {
							analysis_data_struct_ptr->expression_tmp_stack.push(&analysis_data_struct_ptr->node_struct[node_index]);
							node_attribute = &analysis_data_struct_ptr->node_attribute[++node_index];
							analysis_data_struct_ptr->node_struct[node_index].value = node_attribute;
							node_attribute->node_type = TOKEN_OPERATOR;
							node_attribute->value_type = 1;
							node_attribute->data = OPT_L_SMALL_BRACKET;
						} else if(node_attribute->data == OPT_PLUS_PLUS || node_attribute->data == OPT_MINUS_MINUS) {
							node_attribute_t *final_stack_top_ptr = (node_attribute_t*)analysis_data_struct_ptr->expression_final_stack.get_lastest_element()->value;
							if(!analysis_data_struct_ptr->expression_final_stack.get_count() || this->sentence_analysis_data_struct.last_token.node_type == TOKEN_OPERATOR) {
								if(node_attribute->data == OPT_PLUS_PLUS)
									node_attribute->data = OPT_L_PLUS_PLUS;
								else
									node_attribute->data = OPT_L_MINUS_MINUS;
							} else {
								if(node_attribute->data == OPT_PLUS_PLUS)
									node_attribute->data = OPT_R_PLUS_PLUS;
								else
									node_attribute->data = OPT_R_MINUS_MINUS;
							}
						} else if(node_attribute->data == OPT_COMMA) {
							for(int n=node_index-1; n>=0; n--) {
								if(analysis_data_struct_ptr->node_attribute[n].node_type == TOKEN_OPERATOR && analysis_data_struct_ptr->node_attribute[n].data == OPT_L_SMALL_BRACKET) {
									if(n>0 && analysis_data_struct_ptr->node_attribute[n-1].node_type == TOKEN_OPERATOR && analysis_data_struct_ptr->node_attribute[n-1].data == OPT_CALL_FUNC) {
										node_attribute->data = OPT_FUNC_COMMA;
										break;
									}
								}
							}
						} else if(node_attribute->data == OPT_INDEX) {
							analysis_data_struct_ptr->expression_tmp_stack.push(&analysis_data_struct_ptr->node_struct[node_index]);
							node_attribute = &analysis_data_struct_ptr->node_attribute[++node_index];
							analysis_data_struct_ptr->node_struct[node_index].value = node_attribute;
							node_attribute->node_type = TOKEN_OPERATOR;
							node_attribute->value_type = 1;
							node_attribute->data = OPT_L_SMALL_BRACKET;
						}
						analysis_data_struct_ptr->expression_tmp_stack.push(&analysis_data_struct_ptr->node_struct[node_index]);
						break;
					}
				} else {//Notion: priority of right parenthese should be lowwest.
					analysis_data_struct_ptr->expression_final_stack.push(analysis_data_struct_ptr->expression_tmp_stack.pop());
				}
			}
		} else if(node_attribute->node_type == TOKEN_NAME || node_attribute->node_type == TOKEN_CONST_VALUE || node_attribute->node_type == TOKEN_STRING) {
			this->sentence_analysis_data_struct.expression_final_stack.push(&analysis_data_struct_ptr->node_struct[node_index]);
		}
		this->sentence_analysis_data_struct.last_token = *node_attribute;
		len -= token_len;
		str += token_len;
		node_index++;
	}
	while(analysis_data_struct_ptr->expression_tmp_stack.get_count()) {
		analysis_data_struct_ptr->expression_final_stack.push(analysis_data_struct_ptr->expression_tmp_stack.pop());
	}
	if(need_semicolon) {
		node *last_token = analysis_data_struct_ptr->expression_final_stack.pop();
		if(((node_attribute_t*)last_token->value)->node_type != TOKEN_OPERATOR || ((node_attribute_t*)last_token->value)->data != OPT_EDGE) {
			error("Missing ;\n");
			return ERROR_SEMICOLON;
		}
	}
	//后序表达式构造完成，下面构造二叉树
	//while(analysis_data_struct_ptr->expression_final_stack.get_count()) {
	//	node_attribute_t *tmp = (node_attribute_t*)analysis_data_struct_ptr->expression_final_stack.pop()->value;
	//	printf("%d ",tmp->node_type);
	//	if(tmp->node_type == TOKEN_NAME)
	//		printf("%s\n",tmp->value.ptr_value);
	//	else if(tmp->node_type == TOKEN_STRING)
	//		printf("%s\n", tmp->value.ptr_value);
	//	else
	//		printf("%d %d\n",tmp->data,tmp->value_type);
	//}
	node *root = analysis_data_struct_ptr->expression_final_stack.pop();
	if(!root) {
		warning("No token found.\n");
		return 0;//TODO:找个合适的返回值
	}
	this->sentence_analysis_data_struct.tree_root = root;
	int ret;
	if(analysis_data_struct_ptr->expression_final_stack.get_count() == 0) {
		ret = generate_expression_value(this->cur_mid_code_stack_ptr, (node_attribute_t*)root->value);
		if(ret)return ret;
	} else {
		root->link_reset();
		ret = list_stack_to_tree(root, &analysis_data_struct_ptr->expression_final_stack);//二叉树完成
		if(ret)return ret;
		if(analysis_data_struct_ptr->expression_final_stack.get_count()) {
			error("Exist extra token.\n");
			return ERROR_OPERAND_SURPLUS;
		}
		//root->middle_visit();
		ret = this->tree_to_code(root, this->cur_mid_code_stack_ptr);//构造中间代码
		if(ret) {
			while(this->mid_varity_stack.get_count()) {
				varity_info *tmp_varity_ptr = (varity_info*)this->mid_varity_stack.pop();
				dec_varity_ref(tmp_varity_ptr, true);
			}
			return ret;
		}
		if(this->sentence_analysis_data_struct.short_depth) {
			error("? && : unmatch.\n");
			this->sentence_analysis_data_struct.short_depth = 0;
			return ERROR_TERNARY_UNMATCH;
		}
	}
	if(this->mid_varity_stack.get_count()) {
		dec_varity_ref((varity_info*)this->mid_varity_stack.get_base_addr(), true);
		this->mid_varity_stack.pop();
	}
	if(((node_attribute_t*)root->value)->node_type == TOKEN_NAME && ((node_attribute_t*)root->value)->value.ptr_value[0] == LINK_VARITY_PREFIX) {
		generate_expression_value(this->cur_mid_code_stack_ptr, (node_attribute_t*)root->value);
		return ERROR_NO;
	}
	debug("generate code.\n");
	return ERROR_NO;
}

int c_interpreter::generate_expression_value(stack *code_stack_ptr, node_attribute_t *node_attribute)//TODO:加参数，表示生成的赋值到哪个变量里。
{
		mid_code *instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
		varity_info *new_varity_ptr;
		if(node_attribute->node_type == TOKEN_NAME && (node_attribute->value.ptr_value[0] == TMP_VAIRTY_PREFIX || node_attribute->value.ptr_value[0] == LINK_VARITY_PREFIX)) {
			instruction_ptr->opda_addr = 8 * node_attribute->value.ptr_value[1];
		} else {
			instruction_ptr->opda_addr = this->mid_varity_stack.get_count() * 8;
			new_varity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(this->mid_varity_stack.get_count());
			this->mid_varity_stack.push();
		}
		instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_operator = OPT_ASSIGN;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		if(node_attribute->node_type == TOKEN_CONST_VALUE) {
			instruction_ptr->opdb_operand_type = OPERAND_CONST;
			instruction_ptr->opdb_varity_type = node_attribute->value_type;
			kmemcpy(&instruction_ptr->opdb_addr, &node_attribute->value, 8);
			new_varity_ptr->set_type(node_attribute->value_type);
			inc_varity_ref(new_varity_ptr);
		} else if(node_attribute->node_type == TOKEN_NAME) {
			if(node_attribute->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {
				instruction_ptr->opdb_operand_type = OPERAND_T_VARITY;
				instruction_ptr->opdb_addr = 8 * node_attribute->value.ptr_value[1];
				instruction_ptr->opdb_varity_type = ((varity_info*)this->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]))->get_type();
			} else if(node_attribute->value.ptr_value[0] == LINK_VARITY_PREFIX) {//TODO:检查一遍operand_type
				instruction_ptr->opdb_operand_type = OPERAND_LINK_VARITY;
				instruction_ptr->opdb_addr = (int)((varity_info*)this->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]))->get_content_ptr();
				instruction_ptr->opdb_varity_type = ((varity_info*)this->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]))->get_type();
			} else {
					varity_info *varity_ptr;
					int varity_scope;
					varity_ptr = this->varity_declare->vfind(node_attribute->value.ptr_value, varity_scope);
					if(!varity_ptr) {
						this->mid_varity_stack.pop();
						error("Varity not exist.\n");
						return ERROR_VARITY_NONEXIST;
					}
					if(varity_scope == VARITY_SCOPE_GLOBAL)
						instruction_ptr->opdb_operand_type = OPERAND_G_VARITY;
					else
						instruction_ptr->opdb_operand_type = OPERAND_L_VARITY;
					instruction_ptr->opdb_addr = (int)varity_ptr->get_content_ptr();
					instruction_ptr->opdb_varity_type = varity_ptr->get_type();
					new_varity_ptr->set_type(instruction_ptr->opdb_varity_type);
					inc_varity_ref(new_varity_ptr);
			}
		}
		instruction_ptr->opda_varity_type = instruction_ptr->opdb_varity_type;
		instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type;
		instruction_ptr->ret_addr = instruction_ptr->opda_addr;//TODO：需要吗？核对一下handle
		code_stack_ptr->push();
		return ERROR_NO;
}
extern int opt_time;
ITCM_TEXT int c_interpreter::exec_mid_code(mid_code *pc, uint count)
{
	int ret;
	mid_code *end_ptr = pc + count;
	this->pc = pc;
	//int tick1, tick2 = HWREG(0x2040018), total1 = 0, total2 = 0;
	opt_time = 0;
	while(this->pc < end_ptr) {
		//tick1 = HWREG(0x2040018);
		//total1 += tick2 - tick1;
		ret = call_opt_handle(this);
		//tick2 = HWREG(0x2040018);
		//total2 += tick1 - tick2;
		if(ret)
			return ret;
		this->pc++;
	}
	//kprintf("t1=%d,t2=%d,ot=%d\n", total1, total2, opt_time);
	return ERROR_NO;
}

int c_interpreter::sentence_analysis(char* str, int len)
{
	int ret1, ret2;
	ret1 = this->label_analysis(str, len);
	if(ret1 != ERROR_NO)
		return ret1;
	ret1 = struct_analysis(str, len);
	if(ret1 != OK_STRUCT_NOSTRUCT)
		return ERROR_NO;
	ret1 = function_analysis(str, len);
	if(ret1 < 0 || ret1 == OK_FUNC_FINISH)
		return ret1;
	//if(ret != OK_FUNC_NOFUNC)
	//	return ERROR_NO;
	ret2 = non_seq_struct_analysis(str, len);
	//debug("nonseqret=%d\n", ret2);
	if(ret2 == OK_NONSEQ_FINISH || ret2 == OK_NONSEQ_INPUTING || ret2 == OK_NONSEQ_DEFINE) {
		if(this->cur_mid_code_stack_ptr == &this->mid_code_stack) {
			this->varity_global_flag = VARITY_SCOPE_LOCAL;
		}
		if(this->cur_mid_code_stack_ptr == &this->mid_code_stack && ret2 == OK_NONSEQ_FINISH) {
			this->varity_global_flag = VARITY_SCOPE_GLOBAL;
			this->nonseq_info->stack_frame_size = this->varity_declare->local_varity_stack->offset;
			this->varity_declare->destroy_local_varity_cur_depth();
		}
	} else if(ret2 < 0)
		return ret2;
	
	if(nonseq_info->non_seq_exec) {
		debug("exec non seq struct\n");
		nonseq_info->non_seq_exec = 0;
		if(this->exec_flag) {
			this->print_code();
			this->exec_mid_code((mid_code*)this->cur_mid_code_stack_ptr->get_base_addr(), this->cur_mid_code_stack_ptr->get_count());
			//ret = this->non_seq_section_exec(0, nonseq_info->row_num - 1);
			this->cur_mid_code_stack_ptr->empty();
		}
		nonseq_info->reset();
		return ret2;//avoid continue to exec single sentence.
	}
	if(!nonseq_info->non_seq_struct_depth && ret2 != OK_NONSEQ_INPUTING && str[0] != '}' && str[0] != '{' && ret1 != OK_FUNC_DEFINE || ret1 == OK_FUNC_INPUTING && ret2 != OK_NONSEQ_INPUTING && !nonseq_info->non_seq_struct_depth && str[0] != '{') {
	//if(str[0] != '}') {
		ret1 = sentence_exec(str, len, true);
		return ret1;
	}
	return ERROR_NO;
}

int c_interpreter::nonseq_mid_gen_mid_code(char *str, uint len)
{
	mid_code* mid_code_ptr;
	int cur_depth = nonseq_info->row_info_node[nonseq_info->row_num].non_seq_depth;
	mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
	mid_code_ptr->ret_operator = CTL_BRANCH;
	this->cur_mid_code_stack_ptr->push();
	nonseq_info->row_info_node[nonseq_info->row_num].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
	for(int i=nonseq_info->row_num-1; i>=0; i--) {
		if(nonseq_info->row_info_node[i].non_seq_depth == cur_depth + 1
			&& nonseq_info->row_info_node[i].non_seq_info == 0) {
			mid_code_ptr = nonseq_info->row_info_node[i].post_info_b + (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
			mid_code_ptr->opda_addr++;
			nonseq_info->row_info_node[i].finish_flag = 1;
			break;
		}
	}
	return ERROR_NO;
}

int c_interpreter::nonseq_end_gen_mid_code(int row_num, char *str_try, int len_try)
{
	int i, key_len, len;
	char *str;
	if(str_try)
		str = str_try;
	else
		str =  nonseq_info->row_info_node[row_num].row_ptr;
	if(len_try)
		len = len_try;
	else
		len = nonseq_info->row_info_node[row_num].row_len;
	int cur_depth = nonseq_info->row_info_node[row_num].non_seq_depth;
	if(len == 0)return ERROR_NO;
	mid_code *mid_code_ptr;
	node_attribute_t cur_node;
	key_len = get_token(str, &cur_node);
	if(cur_node.node_type != TOKEN_KEYWORD_NONSEQ && str[0] != '}') {//TODO：do while 检测bug
		if(!nonseq_info->row_info_node[nonseq_info->row_num].finish_flag) {
			nonseq_info->row_info_node[nonseq_info->row_num].finish_flag = 1;
		}
	}
	for(i=row_num; i>=0; i--) {
		if(nonseq_info->row_info_node[i].non_seq_depth >= cur_depth 
			&& (nonseq_info->row_info_node[i].non_seq_info == 0 || nonseq_info->row_info_node[i].non_seq_info == 2) 
			&& !nonseq_info->row_info_node[i].finish_flag) {
			switch(nonseq_info->row_info_node[i].nonseq_type) {
			case NONSEQ_KEY_IF:
				mid_code_ptr = nonseq_info->row_info_node[i].post_info_b + (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
				mid_code_ptr->opda_addr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr() - mid_code_ptr;
				nonseq_info->row_info_node[i].finish_flag = 1;
				break;
			case NONSEQ_KEY_ELSE:
				mid_code_ptr = nonseq_info->row_info_node[i].post_info_b + (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
				mid_code_ptr->opda_addr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr() - mid_code_ptr;
				nonseq_info->row_info_node[i].finish_flag = 1;
				break;
			case NONSEQ_KEY_FOR:
			{
				int len = find_ch_with_bracket_level(nonseq_info->row_info_node[i].row_ptr + nonseq_info->row_info_node[i].post_info_c + 1, ')', 0);
				mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
				for(mid_code *j=(mid_code*)cur_mid_code_stack_ptr->get_base_addr()+nonseq_info->row_info_node[i].post_info_b+nonseq_info->row_info_node[i].post_info_a; j<=mid_code_ptr; j++) {
					if(j->ret_operator == CTL_CONTINUE && j->opda_addr == 0)
						j->opda_addr = mid_code_ptr - j;
				}
				this->generate_mid_code(nonseq_info->row_info_node[i].row_ptr + nonseq_info->row_info_node[i].post_info_c + 1, len, false);
				mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
				mid_code_ptr->ret_operator = CTL_BRANCH;
				mid_code_ptr->opda_addr = ((mid_code*)cur_mid_code_stack_ptr->get_base_addr() + nonseq_info->row_info_node[i].post_info_b) - mid_code_ptr;
				this->cur_mid_code_stack_ptr->push();
				mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
				for(mid_code *j=(mid_code*)cur_mid_code_stack_ptr->get_base_addr()+nonseq_info->row_info_node[i].post_info_b+nonseq_info->row_info_node[i].post_info_a; j<=mid_code_ptr; j++) {
					if(j->ret_operator == CTL_BREAK && j->opda_addr == 0)
						j->opda_addr = mid_code_ptr - j;
				}
				mid_code_ptr = (mid_code*)cur_mid_code_stack_ptr->get_base_addr() + nonseq_info->row_info_node[i].post_info_b + nonseq_info->row_info_node[i].post_info_a;
				mid_code_ptr->opda_addr = (mid_code*)cur_mid_code_stack_ptr->get_current_ptr() - mid_code_ptr;
				nonseq_info->row_info_node[i].finish_flag = 1;
				break;
			}
			case NONSEQ_KEY_WHILE:
				mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
				mid_code_ptr->ret_operator = CTL_BRANCH;
				mid_code_ptr->opda_addr = ((mid_code*)cur_mid_code_stack_ptr->get_base_addr() + nonseq_info->row_info_node[i].post_info_b) - mid_code_ptr;
				this->cur_mid_code_stack_ptr->push();
				mid_code_ptr = nonseq_info->row_info_node[i].post_info_b + nonseq_info->row_info_node[i].post_info_a + (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
				mid_code_ptr->opda_addr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr() - mid_code_ptr;
				nonseq_info->row_info_node[i].finish_flag = 1;
				break;
			case NONSEQ_KEY_DO:
				this->generate_mid_code(str + key_len, len - key_len, true);
				mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
				mid_code_ptr->ret_operator = CTL_BRANCH_TRUE;
				mid_code_ptr->opda_addr = ((mid_code*)cur_mid_code_stack_ptr->get_base_addr() + nonseq_info->row_info_node[i].post_info_b) - mid_code_ptr;
				this->cur_mid_code_stack_ptr->push();
				break;
			default:
				break;
			}
		}
	}
	return ERROR_NO;
}

int c_interpreter::nonseq_start_gen_mid_code(char *str, uint len, int non_seq_type)
{
	mid_code *mid_code_ptr;
	node_attribute_t token_node;
	int key_len = get_token(str, &token_node), first_flag_pos, second_flag_pos, third_flag_pos;
	int begin_pos = get_token(str + key_len, &token_node);
	int ret = ERROR_NO;
	uint mid_code_count;
	uint mid_code_count_of_for;
	if((token_node.node_type != TOKEN_OPERATOR || token_node.data != OPT_L_SMALL_BRACKET) && token_node.value.int_value != NONSEQ_KEY_DO && token_node.value.int_value != NONSEQ_KEY_ELSE) {
		error("Lack of bracket\n");
		return ERROR_NONSEQ_GRAMMER;
	}
	mid_code_count = this->cur_mid_code_stack_ptr->get_count();
	switch(non_seq_type) {
	case NONSEQ_KEY_IF:
		ret = this->generate_mid_code(str + key_len, len - key_len, false);
		if(ret)
			break;
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
		mid_code_ptr->ret_operator = CTL_BRANCH_FALSE;
		this->cur_mid_code_stack_ptr->push();
		break;
	case NONSEQ_KEY_FOR: 
		first_flag_pos = find_ch_with_bracket_level(str + key_len, ';', 1);
		second_flag_pos = find_ch_with_bracket_level(str + key_len + first_flag_pos + 1, ';', 0);
		nonseq_info->row_info_node[nonseq_info->row_num].post_info_c = key_len + first_flag_pos + second_flag_pos + 1;
		if(first_flag_pos == -1 || second_flag_pos == -1)
			return ERROR_NONSEQ_GRAMMER;
		ret = this->generate_mid_code(str + key_len + begin_pos, first_flag_pos - begin_pos, false);
		if(ret)
			break;
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();//循环条件判断处中间代码地址
		ret = this->generate_mid_code(str + key_len + first_flag_pos + 1, second_flag_pos, false);
		if(ret)
			break;
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num].post_info_a = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr() - nonseq_info->row_info_node[nonseq_info->row_num].post_info_b;//循环体处距循环条件中间代码处相对偏移
		mid_code_ptr->ret_operator = CTL_BRANCH_FALSE;
		this->cur_mid_code_stack_ptr->push();
		mid_code_count_of_for = this->cur_mid_code_stack_ptr->get_count();
		third_flag_pos = find_ch_with_bracket_level(str + nonseq_info->row_info_node[nonseq_info->row_num].post_info_c + 1, ')', 0);
		ret = this->generate_mid_code(str + nonseq_info->row_info_node[nonseq_info->row_num].post_info_c + 1, third_flag_pos, false);
		while(this->cur_mid_code_stack_ptr->get_count() > mid_code_count_of_for) {
			mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->pop();
			kmemset(mid_code_ptr, 0, sizeof(mid_code));
		}
		break;
	case NONSEQ_KEY_WHILE:
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
		ret = this->generate_mid_code(str + key_len, len - key_len, false);
		if(ret)
			break;
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num].post_info_a = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr() - nonseq_info->row_info_node[nonseq_info->row_num].post_info_b;//循环体处距循环条件中间代码处相对偏移
		mid_code_ptr->ret_operator = CTL_BRANCH_FALSE;
		this->cur_mid_code_stack_ptr->push();
		break;
	case NONSEQ_KEY_DO:
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
		break;
	}
	if(ret) {
		while(this->cur_mid_code_stack_ptr->get_count() > mid_code_count) {
			mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->pop();
			kmemset(mid_code_ptr, 0, sizeof(mid_code));
		}
	}
	return ret;
}

int c_interpreter::struct_analysis(char* str, uint len)
{
	int token_len;
	node_attribute_t node;
	int is_varity_declare;
	token_len = get_token(str, &node);
	if(node.node_type == TOKEN_KEYWORD_TYPE)
		is_varity_declare = node.value.int_value;
	else
		is_varity_declare = 0;
	if(this->struct_info_set.declare_flag) {
		if(this->struct_info_set.struct_begin_flag) {
			struct_info_set.struct_begin_flag = 0;
			if(str[0] != '{' || len != 1) {
				struct_info_set.declare_flag = 0;
				error("struct definition error\n");
				return ERROR_STRUCT_ERROR;
			} else {
				return OK_STRUCT_INPUTING;
			}
		} else {
			if(str[0] == '}') {
				struct_info_set.declare_flag = 0;
				this->struct_declare->current_node->struct_size = make_align(this->struct_info_set.current_offset, PLATFORM_WORD_LEN);
				return OK_STRUCT_FINISH;
				//重写reset，一次保留name，stack，二次全部reset。
				//vfree(this->struct_declare->current_node);
			} else {
				char varity_name[32];
				int type, type_len, remain_len = len, complex_node_count, varity_size, align_size;
				struct_info *struct_info_ptr;
				PLATFORM_WORD *varity_complex_ptr;
				stack *varity_stack_ptr = this->struct_declare->current_node->varity_stack_ptr;
				varity_info* new_node_ptr = (varity_info*)varity_stack_ptr->get_current_ptr();
				type = basic_type_check(str, type_len, struct_info_ptr);
				if(type < 0) {     
					error("arg type error.\n");
					return ERROR_FUNC_ARG_LIST;
				}
				str += type_len;
				remain_len -= type_len;
				while(remain_len > 0) {
					token_len = get_token(str, &node);
					if(token_len > remain_len || node.node_type == TOKEN_ERROR) {//TODO:增加NO_TOKEN
						break;
					}
					type_len = remain_len;
					complex_node_count = get_varity_type(str, type_len, varity_name, type, struct_info_ptr, varity_complex_ptr);
					if(type == STRUCT)
						align_size = PLATFORM_WORD_LEN;
					else
						align_size = get_element_size(complex_node_count, varity_complex_ptr);
					varity_size = get_varity_size(0, varity_complex_ptr, complex_node_count);
					this->struct_info_set.current_offset = make_align(this->struct_info_set.current_offset, align_size);
					new_node_ptr->arg_init(varity_name, type, varity_size, (void*)this->struct_info_set.current_offset);
					new_node_ptr->config_complex_info(complex_node_count, varity_complex_ptr);
					this->struct_info_set.current_offset += varity_size;
					varity_stack_ptr->push();
					str += type_len;
					remain_len -= type_len;
				}
				//this->struct_declare->save_sentence(str, len);
				return OK_STRUCT_INPUTING;
			}
		}
	}
	if(is_varity_declare == STRUCT) {
		char struct_name[32];
		token_len += get_token(str + token_len, &node);
		if(node.node_type != TOKEN_NAME) {
			error("Wrong struct define.\n");
			return ERROR_STRUCT_NAME;
		}
		kstrcpy(struct_name, node.value.ptr_value);
		token_len += get_token(str + token_len, &node);
		if(node.node_type == TOKEN_ERROR) {//1:define 2+:declare
			int symbol_begin_pos, ptr_level = 0;
			int keylen = kstrlen(type_key[is_varity_declare]);
			stack* arg_stack;
			this->struct_info_set.declare_flag = 1;
			this->struct_info_set.struct_begin_flag = 1;
			this->struct_info_set.current_offset = 0;
			int i = keylen + 1;
			symbol_begin_pos = i;
			varity_attribute* arg_node_ptr = (varity_attribute*)vmalloc(sizeof(varity_info) * MAX_VARITY_COUNT_IN_STRUCT);
			arg_stack = (stack*)vmalloc(sizeof(stack));
			stack tmp_stack(sizeof(varity_info), arg_node_ptr, MAX_VARITY_COUNT_IN_STRUCT);
			kmemcpy(arg_stack, &tmp_stack, sizeof(stack));
			this->struct_declare->declare(struct_name, arg_stack);
			return OK_STRUCT_INPUTING;
		}
	}
	return OK_STRUCT_NOSTRUCT;
}

int c_interpreter::basic_type_check(char *str, int &len, struct_info *&struct_info_ptr)
{
	int is_varity_declare, token_len, total_token_len = 0;
	char struct_name[MAX_VARITY_NAME_LEN];
	node_attribute_t node_attribute;
	token_len = get_token(str, &node_attribute);
	str += token_len;
	total_token_len += token_len;
	if(node_attribute.node_type == TOKEN_KEYWORD_TYPE) {
		is_varity_declare = node_attribute.value.int_value;
	} else
		is_varity_declare = ERROR_NO_VARITY_FOUND;
	struct_info *struct_node_ptr = 0;
	if(is_varity_declare > 0) {
		if(is_varity_declare == STRUCT) {
			token_len = get_token(str, &node_attribute);
			if(node_attribute.node_type == TOKEN_NAME) {
				kstrcpy(struct_name, node_attribute.value.ptr_value);
				struct_node_ptr = this->struct_declare->find(struct_name);
				if(!struct_node_ptr) {
					error("There is no struct called %s.\n", struct_name);
					return ERROR_STRUCT_NONEXIST;
				}
			} else {
				error("Wrong struct name.\n");
				return ERROR_STRUCT_NONEXIST;
			}
			struct_info_ptr = struct_node_ptr;
			str += token_len;
			total_token_len += token_len;
		}
		len = total_token_len;
		return is_varity_declare;
	} else {
		return ERROR_NO_VARITY_FOUND;
	}
	return 0;
}

int c_interpreter::get_varity_type(char *str, int &len, char *name, int basic_type, struct_info *info, PLATFORM_WORD *&ret_info)
{
	int is_varity_declare = basic_type, token_len, total_len = 0;
	sentence_analysis_data_struct_t *analysis_data_struct_ptr = &this->sentence_analysis_data_struct;
	node_attribute_t *node_attribute = &analysis_data_struct_ptr->node_attribute[0], *stack_top_node_ptr;
	//解析复杂变量类型///////////////
	int final_stack_count_bak = analysis_data_struct_ptr->expression_final_stack.get_count();
	int tmp_stack_count_bak = analysis_data_struct_ptr->expression_tmp_stack.get_count();
	int node_index = final_stack_count_bak + tmp_stack_count_bak, v_len = len;
	int token_flag = 0, array_flag = 0;
	int varity_basic_type = 0;
	list_stack *cur_stack_ptr = &analysis_data_struct_ptr->expression_tmp_stack;
	this->sentence_analysis_data_struct.last_token.node_type = TOKEN_OPERATOR;
	this->sentence_analysis_data_struct.last_token.data = OPT_EDGE;
	name[0] = '\0';
	while(v_len >= 0) {
		if(v_len == 0) {
			token_len = 0;
			goto varity_end;
		}
		node_attribute = &analysis_data_struct_ptr->node_attribute[node_index];
		analysis_data_struct_ptr->node_struct[node_index].value = node_attribute;
		token_len = get_token(str, node_attribute);
		if(token_len > v_len) {
			token_len = 0;
			goto varity_end;
		}
		if(node_attribute->node_type == TOKEN_ERROR)
			return ERROR_TOKEN_TYPE;
		if(node_attribute->node_type == TOKEN_OPERATOR) {
			stack_top_node_ptr = (node_attribute_t*)analysis_data_struct_ptr->expression_tmp_stack.get_lastest_element()->value;
			if(node_attribute->data == OPT_L_SMALL_BRACKET) {
				analysis_data_struct_ptr->expression_tmp_stack.push(&analysis_data_struct_ptr->node_struct[node_index]);
			} else if(node_attribute->data == OPT_R_SMALL_BRACKET) {
				while(1) {//TODO:确认这里是否需要改token_flag；没有函数指针时不用是可以的，不知道加入以后会不会有问题
					stack_top_node_ptr = (node_attribute_t*)analysis_data_struct_ptr->expression_tmp_stack.get_lastest_element()->value;
					if(stack_top_node_ptr->node_type == TOKEN_OPERATOR && stack_top_node_ptr->data == OPT_L_SMALL_BRACKET)
						break;
					analysis_data_struct_ptr->expression_final_stack.push(analysis_data_struct_ptr->expression_tmp_stack.pop());
				}
				analysis_data_struct_ptr->expression_tmp_stack.pop();
			} else if(node_attribute->data == OPT_L_MID_BRACKET) {
				array_flag = 1;
			} else if(node_attribute->data == OPT_R_MID_BRACKET) {
				array_flag = 0;
				cur_stack_ptr->push(&analysis_data_struct_ptr->node_struct[node_index]);
				node_attribute->data = OPT_INDEX;
				node_attribute->value.int_value = this->sentence_analysis_data_struct.last_token.value.int_value;
			} else if(node_attribute->data == OPT_MUL) {
				static node_attribute_t *ptr_node_ptr;
				//if(this->sentence_analysis_data_struct.last_token.node_type == TOKEN_OPERATOR && this->sentence_analysis_data_struct.last_token.data == OPT_PTR_CONTENT) {
				//	ptr_node_ptr->value_type++;
				//} else {
				cur_stack_ptr->push(&analysis_data_struct_ptr->node_struct[node_index]);
				node_attribute->data = OPT_PTR_CONTENT;
				node_attribute->value_type = 1;
				ptr_node_ptr = node_attribute;
				//}
			} else if(node_attribute->data == OPT_COMMA || node_attribute->data == OPT_EDGE || node_attribute->data == OPT_ASSIGN) {
varity_end:
				while(analysis_data_struct_ptr->expression_tmp_stack.get_count() > tmp_stack_count_bak) {
					analysis_data_struct_ptr->expression_final_stack.push(analysis_data_struct_ptr->expression_tmp_stack.pop());
				}
				int node_count = analysis_data_struct_ptr->expression_final_stack.get_count() - final_stack_count_bak;
				PLATFORM_WORD *complex_info = 0, *cur_complex_info_ptr;
				int basic_info_node_count = 1;
				if(is_varity_declare == STRUCT)
					basic_info_node_count++;
				if(node_count) {
					complex_info = (PLATFORM_WORD*)vmalloc((node_count + 1 + basic_info_node_count) * sizeof(PLATFORM_WORD)); //+基本类型
					cur_complex_info_ptr = complex_info + node_count + basic_info_node_count;
					node *head = analysis_data_struct_ptr->expression_final_stack.get_head();
					for(int n=0; n<node_count; n++) {
						head = head->right;
						node_attribute_t *complex_attribute = (node_attribute_t*)head->value;
						if(complex_attribute->data == OPT_INDEX) {
							*cur_complex_info_ptr = (COMPLEX_ARRAY << COMPLEX_TYPE_BIT) | complex_attribute->value.int_value;
						} else if(complex_attribute->data == OPT_PTR_CONTENT) {
							*cur_complex_info_ptr = (COMPLEX_PTR << COMPLEX_TYPE_BIT);
						}
						cur_complex_info_ptr--;
					}
					*cur_complex_info_ptr-- = (COMPLEX_BASIC << COMPLEX_TYPE_BIT) | is_varity_declare;
					if(is_varity_declare == STRUCT) {
						*cur_complex_info_ptr-- = info->type_info_ptr[1];//TODO:或许有更好办法
					}
					while(analysis_data_struct_ptr->expression_final_stack.get_count() > final_stack_count_bak)
						analysis_data_struct_ptr->expression_final_stack.pop();
					//analysis_data_struct_ptr->expression_final_stack.reset();//避免后续声明错误及影响生成后序表达式
					int type_index = c_interpreter::varity_type_stack.find(node_count + basic_info_node_count, complex_info);
					if(type_index >= 0) {
						ret_info = (PLATFORM_WORD*)c_interpreter::varity_type_stack.type_info_addr[type_index];
						vfree(complex_info);
					} else {
						ret_info = complex_info;
					}
				} else {
					if(is_varity_declare == STRUCT)
						ret_info = info->type_info_ptr;
					else
						ret_info = basic_type_info[is_varity_declare];
				}
				////////
				
				node_index = 0;
				this->sentence_analysis_data_struct.last_token.node_type = TOKEN_OPERATOR;
				this->sentence_analysis_data_struct.last_token.data = node_attribute->data;
				v_len -= token_len;
				str += token_len;
				total_len += token_len;
				len = total_len;
				return basic_info_node_count + node_count;
			} else {
#if !DYNAMIC_ARRAY_EN
				error("Not allowed to use dynamic array.\n");
#endif
			}
		} else if(node_attribute->node_type == TOKEN_NAME) {
			token_flag = 1;
			cur_stack_ptr = &analysis_data_struct_ptr->expression_final_stack;
			kstrcpy(name, node_attribute->value.ptr_value);
		}
		this->sentence_analysis_data_struct.last_token = *node_attribute;
		cur_stack_ptr = &analysis_data_struct_ptr->expression_tmp_stack;
		v_len -= token_len;
		str += token_len;
		total_len += token_len;
		node_index++;
	}
	return ERROR_NO_VARITY_FOUND;
}

int c_interpreter::varity_declare_analysis(char* str, uint len)
{
	int is_varity_declare, key_len, token_len, ret;
	node_attribute_t node;
	char varity_name[32];
	struct_info* struct_node_ptr = 0;
	is_varity_declare = basic_type_check(str, key_len, struct_node_ptr);	
	if(is_varity_declare >= 0) {
		str += key_len;
		int type, type_len, remain_len = len - key_len, complex_node_count, varity_size, align_size;
		PLATFORM_WORD *varity_complex_ptr;
		varity_info *new_varity_ptr;
		while(remain_len > 0) {
			token_len = get_token(str, &node);
			if(token_len > remain_len || node.node_type == TOKEN_ERROR) {//TODO:增加NO_TOKEN
				break;
			}
			type_len = remain_len;
			complex_node_count = get_varity_type(str, type_len, varity_name, is_varity_declare, struct_node_ptr, varity_complex_ptr);
			if(is_varity_declare == STRUCT)
				align_size = PLATFORM_WORD_LEN;//TODO:struct内最长对齐长度
			else
				align_size = get_element_size(complex_node_count, varity_complex_ptr);
			varity_size = get_varity_size(0, varity_complex_ptr, complex_node_count);

			if(this->varity_global_flag == VARITY_SCOPE_GLOBAL) {
				ret = this->varity_declare->declare(VARITY_SCOPE_GLOBAL, varity_name, is_varity_declare, varity_size, complex_node_count, varity_complex_ptr);
				new_varity_ptr = (varity_info*)this->varity_declare->global_varity_stack->get_lastest_element();
			} else {
				ret = this->varity_declare->declare(VARITY_SCOPE_LOCAL, varity_name, is_varity_declare, varity_size, complex_node_count, varity_complex_ptr);
				new_varity_ptr = (varity_info*)this->varity_declare->local_varity_stack->get_lastest_element();
			}
			if(!varity_complex_ptr[0]) {
				varity_complex_ptr[0] = 1;//起始引用次数1
			}
			str += type_len;
			remain_len -= type_len;
			if(this->sentence_analysis_data_struct.last_token.data == OPT_ASSIGN) {
				int exp_len = find_ch_with_bracket_level(str, ',', 0);
				if(exp_len == -1)
					exp_len = find_ch_with_bracket_level(str, ';', 0);
				if(new_varity_ptr->get_type() != ARRAY) {
					generate_mid_code(str, exp_len, false);
					mid_code *code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
					kmemcpy(&code_ptr->opdb_addr, &(code_ptr - 1)->opda_addr, sizeof(code_ptr->opda_addr) + sizeof(code_ptr->opda_operand_type) + sizeof(code_ptr->double_space1) + sizeof(code_ptr->opda_varity_type));
					code_ptr->ret_addr = code_ptr->opda_addr = (int)new_varity_ptr->get_content_ptr();
					code_ptr->ret_operand_type = code_ptr->opda_operand_type = this->varity_global_flag;
					code_ptr->ret_varity_type = code_ptr->opda_varity_type = new_varity_ptr->get_type();
					code_ptr->data = new_varity_ptr->get_element_size();
					code_ptr->ret_operator = OPT_ASSIGN;
					this->cur_mid_code_stack_ptr->push();
				} else {//数组赋值
				}
				str += exp_len + 1;
				remain_len -= exp_len + 1;
			}
		}
		return OK_VARITY_DECLARE;
	}
	return ERROR_NO;
}

int c_interpreter::sentence_exec(char* str, uint len, bool need_semicolon)
{
	int ret;
	//int total_bracket_depth;
	if(str[0] == '{' || str[0] == '}')
		return 0;
	if(str[len-1] != ';' && need_semicolon) {
		error("Missing ;\n");
		return ERROR_SEMICOLON;
	}
	//total_bracket_depth = get_bracket_depth(str);
	//if(total_bracket_depth < 0) {
	//	error("Bracket unmatch.\n");
	//	return ERROR_BRACKET_UNMATCH;
	//}
	int key_word_ret = varity_declare_analysis(str, len);
	if(!key_word_ret) {
		ret = this->generate_mid_code(str, len, true);
		if(ret) return ret;
	}
	if(this->exec_flag) {
		int mid_code_count = this->cur_mid_code_stack_ptr->get_count();
		this->print_code();
		this->exec_mid_code((mid_code*)this->cur_mid_code_stack_ptr->get_base_addr(), mid_code_count);
		this->cur_mid_code_stack_ptr->empty();
	}
	return ERROR_NO;
}

int c_interpreter::non_seq_struct_check(char* str)
{
	return nonseq_key_cmp(str);
}

void c_interpreter::print_code(void)
{
	int n = this->cur_mid_code_stack_ptr->get_count();
	mid_code *ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
	for(int i=0; i<n; i++, ptr++) {
		if(ptr->ret_operator >= 100) {
			debug("%d ", ptr->ret_operator);
		}
		if(ptr->ret_operand_type == OPERAND_T_VARITY) {
			debug("$%d=", ptr->ret_addr / 8);
		} else if(ptr->ret_operand_type == OPERAND_LINK_VARITY) {
			debug("#%d=", ptr->ret_addr / 8);
		} else if(ptr->ret_operand_type == OPERAND_G_VARITY) {
			for(uint i=0; i<this->language_elment_space.g_varity_list.get_count(); i++) {
				if(this->language_elment_space.g_varity_node[i].get_content_ptr() == (void*)ptr->ret_addr) {
					debug("%s=",this->language_elment_space.g_varity_node[i].get_name());
					break;
				}
			}
		} else if(ptr->ret_operand_type == OPERAND_L_VARITY) {
			debug("SP+%d=", ptr->ret_addr);
		}
		if(ptr->opda_operand_type == OPERAND_T_VARITY) {
			debug("$%d ", ptr->opda_addr / 8);
		} else if(ptr->opda_operand_type == OPERAND_LINK_VARITY) {
			debug("#%d ", ptr->opda_addr / 8);
		} else if(ptr->opda_operand_type == OPERAND_G_VARITY) {
			for(uint i=0; i<this->language_elment_space.g_varity_list.get_count(); i++) {
				if(this->language_elment_space.g_varity_node[i].get_content_ptr() == (void*)ptr->opda_addr) {
					debug("%s ",this->language_elment_space.g_varity_node[i].get_name());
					break;
				}
			}
		} else if(ptr->opda_operand_type == OPERAND_L_VARITY) {
			debug("SP+%d ", ptr->opda_addr);
		} else if(ptr->opda_operand_type == OPERAND_CONST) {
			if(ptr->opda_varity_type == INT || ptr->opda_varity_type == SHORT || ptr->opda_varity_type == LONG || ptr->opda_varity_type == CHAR) {
				debug("%d ", ptr->opda_addr);
			} else if(ptr->opda_varity_type == U_INT || ptr->opda_varity_type == U_SHORT || ptr->opda_varity_type == U_LONG || ptr->opda_varity_type == U_CHAR) {
				debug("%lu ", ptr->opda_addr);
			}
		}
		if(ptr->ret_operator < 100) {
			debug("%d ", ptr->ret_operator);
		}
		if(ptr->opdb_operand_type == OPERAND_T_VARITY) {
			debug("$%d", ptr->opdb_addr / 8);
		} else if(ptr->opdb_operand_type == OPERAND_LINK_VARITY) {
			debug("#%d", ptr->opdb_addr / 8);
		} else if(ptr->opdb_operand_type == OPERAND_G_VARITY) {
			for(uint i=0; i<this->language_elment_space.g_varity_list.get_count(); i++) {
				if(this->language_elment_space.g_varity_node[i].get_content_ptr() == (void*)ptr->opdb_addr) {
					debug("%s",this->language_elment_space.g_varity_node[i].get_name());
					break;
				}
			}
		} else if(ptr->opdb_operand_type == OPERAND_L_VARITY) {
			debug("SP+%d", ptr->opdb_addr);
		} else if(ptr->opdb_operand_type == OPERAND_CONST) {
			if(ptr->opdb_varity_type == INT || ptr->opdb_varity_type == SHORT || ptr->opdb_varity_type == LONG || ptr->opdb_varity_type == CHAR) {
				debug("%d", ptr->opdb_addr);
			} else if(ptr->opdb_varity_type == U_INT || ptr->opdb_varity_type == U_SHORT || ptr->opdb_varity_type == U_LONG || ptr->opdb_varity_type == U_CHAR) {
				debug("%lu", ptr->opdb_addr);
			}
		}
		debug("\n");
		//debug("opt=%d,radd=%x,rtype=%d,ropd=%d,aadd=%x,atype=%d,aopd=%d,badd=%x,byte=%d,bopd=%d\n",ptr->ret_operator,ptr->ret_addr,ptr->ret_varity_type,ptr->ret_operand_type,ptr->opda_addr,ptr->opda_varity_type,ptr->opda_operand_type,ptr->opdb_addr,ptr->opdb_varity_type,ptr->opdb_operand_type);
	}
}

extern "C" void run_interpreter(void)
{
	kprintf("\nC program language interpreter.\n");
	myinterpreter.run_interpreter();
}

extern round_queue token_fifo;
extern "C" void uc_timer_init(unsigned int index);
extern "C" void global_init(void)
{
	heapinit();
	token_fifo.init(MAX_TOKEN_BUFLEN);
	//uc_timer_init(1);
}
