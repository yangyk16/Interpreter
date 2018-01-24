#include "interpreter.h"
#include <stdio.h>
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
varity_info g_varity_node[MAX_G_VARITY_NODE];
varity_info l_varity_node[MAX_L_VARITY_NODE];
indexed_stack l_varity_list(sizeof(varity_info), l_varity_node, MAX_L_VARITY_NODE);
stack g_varity_list(sizeof(varity_info), g_varity_node, MAX_G_VARITY_NODE);
varity c_varity(&g_varity_list, &l_varity_list);
nonseq_info_struct nonseq_info_s;
function_info function_node[MAX_FUNCTION_NODE];
stack function_list(sizeof(function_info), function_node, MAX_FUNCTION_NODE);
function c_function(&function_list);
struct_info struct_node[MAX_STRUCT_NODE];
stack struct_list(sizeof(struct_info), struct_node, MAX_STRUCT_NODE);
struct_define c_struct(&struct_list);
c_interpreter myinterpreter;

char non_seq_key[][7] = {"", "if", "switch", "else", "for", "while", "do"};
const char non_seq_key_len[] = {0, 2, 6, 4, 3, 5, 2};
char opt_str[43][4] = {"<<=",">>=","->","++","--","<<",">>",">=","<=","==","!=","&&","||","/=","*=","%=","+=","-=","&=","^=","|=","[","]","(",")",".","-","~","*","&","!","/","%","+",">","<","^","|","?",":","=",",",";"};
const char opt_str_len[] = {3,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
const char opt_prio[] ={14,14,1,2,2,5,5,6,6,7,7,11,12,14,14,14,14,14,14,14,14,1,1,1,17,1,4,2,3,8,2,3,3,4,6,6,9,10,13,13,14,15,16};
const char opt_number[] = {2,2,2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,2,2,2,1,1,1,1};
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
			ret = list_stack_to_tree(last_node, post_order_stack);
			if(ret)
				return ret;
		} else if(last_node_attribute->node_type == TOKEN_NAME || last_node_attribute->node_type == TOKEN_CONST_VALUE || last_node_attribute->node_type == TOKEN_STRING) {
		} else {
			error("Invalid key word.\n");
			return ERROR_INVALID_KEYWORD;
		}
		if(opt_number[((node_attribute_t*)tree_node->value)->value.int_value] == 1 && ((node_attribute_t*)tree_node->value)->node_type == TOKEN_OPERATOR) {
			return ERROR_NO;
		}
		//TODO: 函数查参数个数, 避免使用全局function数组，应把全局量改为类的静态成员变量
		if(((node_attribute_t*)tree_node->value)->node_type == TOKEN_OPERATOR && ((node_attribute_t*)tree_node->value)->value.int_value == OPT_CALL_FUNC) {
			function_info *function_ptr = this->function_declare->find(((node_attribute_t*)tree_node->value - 1)->value.ptr_value);
			if(!function_ptr) {
				error("Function not found.\n");
				return ERROR_VARITY_NONEXIST; //TODO: 找一个合适的错误码
			}
			if(function_ptr->arg_list->get_count() == 1) {
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
	register varity_info *avarity_ptr = 0, *bvarity_ptr = 0, *rvarity_ptr;
	register int opt = ((node_attribute_t*)opt_node_ptr->value)->value.int_value;
	register int varity_scope;
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
						error("Varity not exist\n");
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
						error("Varity not exist\n");
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
		struct_info *struct_info_ptr = (struct_info*)(((int*)avarity_ptr->get_complex_ptr())[1]);
		//ret_type = get_ret_type(instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type);
		varity_number = this->mid_varity_stack.get_count();
		this->mid_varity_stack.push();
		member_varity_ptr = (varity_info*)struct_info_ptr->varity_stack_ptr->find(node_attribute->value.ptr_value);
		avarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		avarity_ptr->set_type(member_varity_ptr->get_type());
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
		((node_attribute_t*)opt_node_ptr->value)->value_type = varity_scope;
		break;
	}
	case OPT_INDEX:
	{//TODO:set_type也需要重写与声明统一
		varity_number = this->mid_varity_stack.get_count();
		this->mid_varity_stack.push();
		uint *complex_info_ptr = (uint*)avarity_ptr->get_complex_ptr();
		int complex_arg_count = avarity_ptr->get_complex_arg_count();
		if(GET_COMPLEX_TYPE(complex_info_ptr[complex_arg_count]) == COMPLEX_ARRAY || GET_COMPLEX_TYPE(complex_info_ptr[complex_arg_count]) == COMPLEX_PTR) {
			rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
			rvarity_ptr->set_size(get_varity_size(GET_COMPLEX_DATA(complex_info_ptr[0]), complex_info_ptr, complex_arg_count - 1));
			rvarity_ptr->config_complex_info(complex_arg_count - 1, complex_info_ptr);
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
			return ERROR_USED_INDEX;
		}
		break;
	}
	case OPT_PLUS:
	case OPT_MINUS:
	case OPT_MUL:
	case OPT_DIVIDE:
		if(opt == OPT_PLUS || opt == OPT_MINUS)//TODO:减法可用两个指针减，改用cmp的try_handle
			ret_type = try_call_opt_handle(OPT_PLUS, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, avarity_ptr->get_complex_arg_count(), (int*)avarity_ptr->get_complex_ptr(), bvarity_ptr->get_complex_arg_count(), (int*)bvarity_ptr->get_complex_ptr());
		else
			ret_type = try_call_opt_handle(OPT_MUL, instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type, avarity_ptr->get_complex_arg_count(), (int*)avarity_ptr->get_complex_ptr(), bvarity_ptr->get_complex_arg_count(), (int*)bvarity_ptr->get_complex_ptr());
		if(ret_type < 0)
			return ret_type;
		varity_number = this->mid_varity_stack.get_count();
		this->mid_varity_stack.push();
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(ret_type);
		if(ret_type == PTR) {
			if(instruction_ptr->opda_varity_type >= PTR && instruction_ptr->opdb_varity_type <= VOID) {
				rvarity_ptr->config_complex_info(avarity_ptr->get_complex_arg_count(), avarity_ptr->get_complex_ptr());
				instruction_ptr->data = avarity_ptr->get_first_order_sub_struct_size();
			} else if(instruction_ptr->opda_varity_type <= VOID && instruction_ptr->opdb_varity_type >= PTR) {
				rvarity_ptr->config_complex_info(bvarity_ptr->get_complex_arg_count(), bvarity_ptr->get_complex_ptr());
				instruction_ptr->data = bvarity_ptr->get_first_order_sub_struct_size();
			}/* else {
				error("Wrong varities type for operator. Interpreter.cpp Line:%d\n", __LINE__);
				return ERROR_ILLEGAL_OPERAND;
			}*/
			array_to_ptr((PLATFORM_WORD*&)rvarity_ptr->get_complex_ptr(), rvarity_ptr->get_complex_arg_count());
		}
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
		if(ret_type < 0)
			return ret_type;
		ret_type = INT;
		varity_number = this->mid_varity_stack.get_count();
		this->mid_varity_stack.push();
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(ret_type);
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
			return ERROR_ILLEGAL_OPERAND;
		}
	case OPT_ASSIGN:
	case OPT_MUL_ASSIGN:
	case OPT_ADD_ASSIGN:
	case OPT_MINUS_ASSIGN:
	case OPT_DEVIDE_ASSIGN:
		ret_type = instruction_ptr->opda_varity_type;
		if(instruction_ptr->opda_operand_type == OPERAND_T_VARITY) {
			error("Assign operator need left value.\n");
			return ERROR_NEED_LEFT_VALUE;
		} else if(instruction_ptr->opdb_operand_type == OPERAND_T_VARITY) {
			//this->mid_varity_stack.pop();
		} else {
		}
		instruction_ptr->ret_addr = instruction_ptr->opda_addr;
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type;
		instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type;
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = ((node_attribute_t*)opt_node_ptr->left->value)->value.ptr_value;
		break;
	case OPT_L_PLUS_PLUS:
	case OPT_L_MINUS_MINUS:
		if(instruction_ptr->opdb_operand_type == OPERAND_T_VARITY) {
			error("++ operator need left value.\n");
			return ERROR_NEED_LEFT_VALUE;
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
			return ERROR_NEED_LEFT_VALUE;
		}
		varity_number = this->mid_varity_stack.get_count();
		this->mid_varity_stack.push();
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->config_complex_info(bvarity_ptr->get_complex_arg_count(), bvarity_ptr->get_complex_ptr());
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
	case OPT_ADDRESS_OF://TODO:变量声明时，基本变量也设置complex_info，增加complex_info最大容限，避免无限取地址出错，在声明时可预留1次取地址扩展，取址时先验证扩展位是否为0再扩展，否则覆盖其他变量类型。
	{
		varity_number = this->mid_varity_stack.get_count();
		this->mid_varity_stack.push();
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
		inc_varity_ref(rvarity_ptr);
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		break;
	}
	case OPT_PTR_CONTENT:
	{
		varity_number = this->mid_varity_stack.get_count();
		this->mid_varity_stack.push();
		//bvarity被rvarity覆盖，先处理，都要留意获取rvarity之后的覆盖问题
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->config_complex_info(bvarity_ptr->get_complex_arg_count() - 1, bvarity_ptr->get_complex_ptr());
		rvarity_ptr->set_size(get_varity_size(0, (uint*)bvarity_ptr->get_complex_ptr(), rvarity_ptr->get_complex_arg_count()));
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
		if(ret_type < 0)
			return ret_type;
	case OPT_NOT:
	case OPT_BIT_REVERT:
	case OPT_POSITIVE:
	case OPT_NEGATIVE:
		varity_number = this->mid_varity_stack.get_count();
		this->mid_varity_stack.push();
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(INT);
		inc_varity_ref(rvarity_ptr);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = INT;
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
		this->sentence_analysis_data_struct.short_calc_stack[this->sentence_analysis_data_struct.short_depth] = (int)(instruction_ptr - 1);
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
		this->mid_varity_stack.push();
		rvarity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		rvarity_ptr->set_type(ret_type);
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
		return ERROR_NO;
	}
	case OPT_CALL_FUNC:
	case OPT_FUNC_COMMA:
		if(opt_node_ptr->right) {
			node_attribute = (node_attribute_t*)opt_node_ptr->right->value;
			if(node_attribute->node_type == TOKEN_NAME || node_attribute->node_type == TOKEN_CONST_VALUE || node_attribute->node_type == TOKEN_STRING) {
				stack *arg_list_ptr = (stack*)this->call_func_info.function_ptr[this->call_func_info.function_depth - 1]->arg_list;
				instruction_ptr->ret_operator = OPT_PASS_PARA;
				instruction_ptr->ret_operand_type = instruction_ptr->opda_operand_type = OPERAND_L_S_VARITY;
				instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
				if(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] >= this->call_func_info.arg_count[this->call_func_info.function_depth - 1] - 1) {
					if(!this->call_func_info.function_ptr[this->call_func_info.function_depth - 1]->variable_para_flag) {
						error("Too many parameters\n");
						return ERROR_FUNC_ARGS;
					} else {
						if(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] == this->call_func_info.arg_count[this->call_func_info.function_depth - 1]) {
							this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
						}
						if(instruction_ptr->opdb_varity_type <= U_LONG && instruction_ptr->opdb_varity_type >= CHAR) {//TODO:平台相关，应该换掉
							instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = INT;
							this->call_func_info.offset[this->call_func_info.function_depth - 1] += 4;
						} else if(instruction_ptr->opdb_varity_type == FLOAT || instruction_ptr->opdb_varity_type == DOUBLE) {
							instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = DOUBLE;
							this->call_func_info.offset[this->call_func_info.function_depth - 1] += 8;
						} else if(instruction_ptr->opdb_varity_type == PTR) {
							instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = PLATFORM_TYPE;
							this->call_func_info.offset[this->call_func_info.function_depth - 1] += PLATFORM_WORD_LEN;
						} else {
							instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = LONG_LONG;
							this->call_func_info.offset[this->call_func_info.function_depth - 1] += 8;
						}
					}
				} else {
					varity_info *arg_varity_ptr = (varity_info*)(arg_list_ptr->visit_element_by_index(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1]));
					instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = arg_varity_ptr->get_type();
					this->call_func_info.offset[this->call_func_info.function_depth - 1] += arg_varity_ptr->get_size();
				}

				this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1]++;

				if(instruction_ptr->opdb_operand_type == OPERAND_T_VARITY || instruction_ptr->opdb_operand_type == OPERAND_LINK_VARITY) {
					this->mid_varity_stack.pop();
					//dec_varity_ref(bvarity_ptr, true);//TODO:确认是否有需要，不注释掉出bug了，释放了全局变量类型信息
				}
			}
		}
		if(opt == OPT_CALL_FUNC) {
			if(opt_node_ptr->right && !(node_attribute->value.int_value == OPT_FUNC_COMMA && node_attribute->node_type == TOKEN_OPERATOR))
				code_stack_ptr->push();
			function_info *function_ptr;
			instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
			node_attribute = (node_attribute_t*)opt_node_ptr->left->value;
			function_ptr = this->function_declare->find(node_attribute->value.ptr_value);
			instruction_ptr->opda_addr = (int)function_ptr;
			instruction_ptr->ret_operator = opt;
			instruction_ptr->ret_varity_type = ((varity_info*)function_ptr->arg_list->visit_element_by_index(0))->get_type();
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
			rvarity_ptr->set_type(instruction_ptr->ret_varity_type);
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
	case OPT_EDGE:
		error("Extra ;\n");
		return ERROR_SEMICOLON;
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
	switch(node_attribute->value.int_value) {
		varity_info *varity_ptr;
	case OPT_TERNARY_Q:
		this->generate_expression_value(code_stack_ptr, (node_attribute_t*)opt_node_ptr->left->value);
		this->mid_varity_stack.pop();
		varity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(this->mid_varity_stack.get_count());
		dec_varity_ref(varity_ptr, true);
		((node_attribute_t*)opt_node_ptr->left->value)->node_type = TOKEN_CONST_VALUE;
		instruction_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		instruction_ptr->ret_operator = CTL_BRANCH_FALSE;
		this->sentence_analysis_data_struct.short_calc_stack[this->sentence_analysis_data_struct.short_depth++] = (int)instruction_ptr;
		code_stack_ptr->push();
		break;
	case OPT_CALL_FUNC:
		this->call_func_info.function_ptr[this->call_func_info.function_depth] = this->function_declare->find(((node_attribute_t*)opt_node_ptr->left->value)->value.ptr_value);
		if(!this->call_func_info.function_ptr[this->call_func_info.function_depth]) {
			error("Function not found.\n");
			return ERROR_VARITY_NONEXIST; //TODO: 找一个合适的错误码
		}
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
						error("Varity not exist\n");
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
			instruction_ptr->ret_addr = instruction_ptr->opda_addr = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
			if(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] >= this->call_func_info.arg_count[this->call_func_info.function_depth - 1] - 1) {
				if(!this->call_func_info.function_ptr[this->call_func_info.function_depth - 1]->variable_para_flag) {
					error("Too many parameters\n");
					return ERROR_FUNC_ARGS;
				} else {
					if(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1] == this->call_func_info.arg_count[this->call_func_info.function_depth - 1]) {
						this->call_func_info.offset[this->call_func_info.function_depth - 1] = make_align(this->call_func_info.offset[this->call_func_info.function_depth - 1], PLATFORM_WORD_LEN);
					}
					if(instruction_ptr->opdb_varity_type <= U_LONG && instruction_ptr->opdb_varity_type >= CHAR) {//TODO:平台相关，应该换掉
						instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = INT;
						this->call_func_info.offset[this->call_func_info.function_depth - 1] += 4;
					} else if(instruction_ptr->opdb_varity_type == FLOAT || instruction_ptr->opdb_varity_type == DOUBLE) {
						instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = DOUBLE;
						this->call_func_info.offset[this->call_func_info.function_depth - 1] += 8;
					} else if(instruction_ptr->opdb_varity_type == PTR) {
						instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = PLATFORM_TYPE;
						this->call_func_info.offset[this->call_func_info.function_depth - 1] += PLATFORM_WORD_LEN;
					} else {
						instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = LONG_LONG;
						this->call_func_info.offset[this->call_func_info.function_depth - 1] += 8;
					}
				}
			} else {
				arg_varity_ptr = (varity_info*)(arg_list_ptr->visit_element_by_index(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1]));
				instruction_ptr->ret_varity_type = instruction_ptr->opda_varity_type = arg_varity_ptr->get_type();
				this->call_func_info.offset[this->call_func_info.function_depth - 1] += arg_varity_ptr->get_size();
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
		this->sentence_analysis_data_struct.short_calc_stack[this->sentence_analysis_data_struct.short_depth++] = (int)this->cur_mid_code_stack_ptr->get_current_ptr();
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
		this->sentence_analysis_data_struct.short_calc_stack[this->sentence_analysis_data_struct.short_depth++] = (int)this->cur_mid_code_stack_ptr->get_current_ptr();
		code_stack_ptr->push();
		break;
	}
	return ERROR_NO;
}

int c_interpreter::operator_pre_handle(stack *code_stack_ptr, node *opt_node_ptr)
{
	register mid_code *instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
	node_attribute_t *node_attribute = ((node_attribute_t*)opt_node_ptr->value);
	switch(node_attribute->value.int_value) {
	case OPT_TERNARY_C:
		if(((node_attribute_t*)opt_node_ptr->left->value)->value.int_value != OPT_TERNARY_Q) {
			error("? && : unmatch.\n");
			return ERROR_TERNARY_UNMATCH;
		}
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

int c_interpreter::get_token(char *str, node_attribute_t *info)
{
	int i = 0, real_token_pos;
	char* symbol_ptr = this->token_fifo.wptr + (char*)this->token_fifo.get_base_addr();//only element_size=1
	while(str[i] == ' ' || str[i] == '\t')i++;
	real_token_pos = i;
	if(is_letter(str[i])) {
		i++;
		while(is_valid_c_char(str[i]))i++;
		this->token_fifo.write(str + real_token_pos, i - real_token_pos);
		this->token_fifo.write("\0",1);
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
		info->value.ptr_value = symbol_ptr;
		info->node_type = TOKEN_NAME;
		return i;
	} else if(is_number(str[i])) {
		i++;
		while(is_number(str[i]))i++;
		info->value_type = INT;
		info->value.int_value = y_atoi(str, i);
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

	} else {
		for(int j=0; j<sizeof(opt_str)/sizeof(opt_str[0]); j++) {
			if(!strmcmp(str + i, opt_str[j], opt_str_len[j])) {
				info->value.int_value = j;
				info->node_type = TOKEN_OPERATOR;
				info->value_type = opt_prio[j];
				return i + opt_str_len[j];
			}
		}
		info->node_type = TOKEN_ERROR;
		return ERROR_TOKEN;
	}
	return 0;
}

int c_interpreter::test(char *str, uint len)
{
	int token_len;
	node_attribute_t node_attribute;
	while(len > 0) {
		token_len = this->get_token(str, &node_attribute);
		len -= token_len;
		str += token_len;
	}
	return 0;
}

int c_interpreter::pre_treat(void)
{
	int spacenum = 0;
	int rptr = 0, wptr = 0, first_word = 1, space_flag = 0;
	char nowchar;
	while(nowchar = sentence_buf[rptr]) {
		if(IsSpace(nowchar)) {
			if(!first_word) {
				if(!space_flag)
					sentence_buf[wptr++] = ' ';
				space_flag = 1;
			}
		} else {
			space_flag = 0;
			first_word = 0;
			sentence_buf[wptr++] = sentence_buf[rptr];
		}
		rptr++;
	}
	sentence_buf[wptr] = 0;
	//this->tty_used->puts(sentence_buf);
	//this->tty_used->puts("\n");
	return wptr;
}

int c_interpreter::run_interpreter(void)
{
	this->init(&stdio, &c_varity, &nonseq_info_s, &c_function, &c_struct);
	this->generate_compile_func();
	while(1) {
		uint len;
		kprintf(">> ");
		len = this->row_pretreat_fifo.readline(sentence_buf);
		if(len > 0) {

		} else {
			tty_used->readline(sentence_buf);
			len = pre_treat();
		}
		this->sentence_analysis(sentence_buf, len);
	}
}

int c_interpreter::init(terminal* tty_used, varity* varity_declare, nonseq_info_struct* nonseq_info, function* function_declare, struct_define* struct_declare)
{
	this->tty_used = tty_used;
	this->varity_declare = varity_declare;
	this->nonseq_info = nonseq_info;
	this->function_declare = function_declare;
	this->struct_declare = struct_declare;
	this->row_pretreat_fifo.set_base(this->pretreat_buf);
	this->row_pretreat_fifo.set_length(sizeof(this->pretreat_buf));
	this->non_seq_code_fifo.set_base(this->non_seq_tmp_buf);
	this->non_seq_code_fifo.set_length(sizeof(this->non_seq_tmp_buf));
	this->non_seq_code_fifo.set_element_size(1);
	this->token_fifo.init(MAX_ANALYSIS_BUFLEN);
	this->nonseq_info->nonseq_begin_stack_ptr = 0;
	this->call_func_info.function_depth = 0;
	this->varity_global_flag = VARITY_SCOPE_GLOBAL;
	///////////
	handle_init();
	this->stack_pointer = this->simulation_stack;
	this->tmp_varity_stack_pointer = this->tmp_varity_stack;
	this->mid_code_stack.init(sizeof(mid_code), MAX_MID_CODE_COUNT);
	this->mid_varity_stack.init(sizeof(varity_info), this->tmp_varity_stack_pointer, MAX_A_VARITY_NODE);//TODO: 设置node最大count
	this->cur_mid_code_stack_ptr = &this->mid_code_stack;
	this->exec_flag = true;
	this->sentence_analysis_data_struct.short_depth = 0;
	this->sentence_analysis_data_struct.label_count = 0;
	for(int i=0; i<MAX_A_VARITY_NODE; i++) {
		link_varity_name[i][0] = LINK_VARITY_PREFIX;
		tmp_varity_name[i][0] = TMP_VAIRTY_PREFIX;
		tmp_varity_name[i][1] = link_varity_name[i][1] = i;
		tmp_varity_name[i][2] = link_varity_name[i][2] = 0;
	}
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
int c_interpreter::struct_end(int struct_end_flag, bool &exec_flag_bak)
{
	int ret = 0;
	while(this->nonseq_info->non_seq_struct_depth > 0 
		&& ((GET_BRACE_DATA(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1]) == nonseq_info->brace_depth + 1 && GET_BRACE_FLAG(nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1]))
		|| (GET_BRACE_DATA(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1]) == nonseq_info->brace_depth && !GET_BRACE_FLAG(nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1])))) {
		this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth - 1] = 0;
		this->varity_declare->local_varity_stack->dedeep();
		if(nonseq_info->non_seq_struct_depth > 0 && nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] == NONSEQ_KEY_IF && !(struct_end_flag & 2)) {
			nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] = NONSEQ_KEY_WAIT_ELSE;
			--nonseq_info->non_seq_struct_depth;
			break;
		}
		--nonseq_info->non_seq_struct_depth;
		if(nonseq_info->non_seq_struct_depth >= 0)
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
			nonseq_info->non_seq_exec = 1;
			this->exec_flag = exec_flag_bak;
			ret = OK_NONSEQ_FINISH;
		}
	}
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
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 3;
			}
		} else {
			if(nonseq_info->non_seq_check_ret != NONSEQ_KEY_ELSE && len != 0) {//TODO:类型于函数中的if，对函数中的if处理可照此执行
				struct_end_flag = 2;
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_IF;
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth = nonseq_info->non_seq_struct_depth + 1;
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 1;
				struct_end(struct_end_flag, exec_flag_bak);
				struct_end_flag = 0;
			} else if(nonseq_info->non_seq_check_ret == NONSEQ_KEY_ELSE) {
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
	if(nonseq_info->last_non_seq_check_ret && nonseq_info->non_seq_struct_depth && str[len-1] == ';') {// && nonseq_info->brace_depth == 0
		if(nonseq_info->last_non_seq_check_ret != NONSEQ_KEY_DO)
			struct_end_flag = 1;
		else
			nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth - 1] = NONSEQ_KEY_WAIT_WHILE;
	}

	if(nonseq_info->non_seq_check_ret) {
		if(nonseq_info->row_num == 0) {
			exec_flag_bak = this->exec_flag;
			this->exec_flag = 0;
		}
		this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] = SET_BRACE(0, nonseq_info->brace_depth);
		this->varity_declare->local_varity_stack->endeep();
		nonseq_info->row_info_node[nonseq_info->row_num].non_seq_info = 0;
		if(nonseq_info->non_seq_check_ret == NONSEQ_KEY_ELSE) {
			nonseq_info->row_info_node[nonseq_info->row_num].non_seq_info = 2;
			if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] != NONSEQ_KEY_WAIT_ELSE) {//|| nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] != nonseq_info->brace_depth
				error("else is unmatch with if\n");
				return ERROR_NONSEQ_GRAMMER;
			}
			nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_IF;
		}
		nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = nonseq_info->non_seq_check_ret;
		nonseq_info->non_seq_struct_depth++;
		nonseq_info->row_info_node[nonseq_info->row_num].non_seq_depth = nonseq_info->non_seq_struct_depth;
		nonseq_info->row_info_node[nonseq_info->row_num].nonseq_type = nonseq_info->non_seq_check_ret;
		ret = OK_NONSEQ_DEFINE;
	}
struct_end_check:
	if(struct_end_flag) {
		ret = struct_end(struct_end_flag, exec_flag_bak);
	}
struct_save:
	if(len && (nonseq_info->non_seq_check_ret || nonseq_info->non_seq_struct_depth)) {
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
	this->function_declare->add_compile_func("kmemset", (void*)kmemset, &memset_stack, 0);
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
			if(node.value.int_value == OPT_MUL) {
				ptr_flag = 1;
			} else {
				varity_info::init_varity(varity_ptr, 0, type, 4);
				varity_ptr++->config_complex_info(1 + ptr_flag, basic_type_info[type]);
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
	ret_function_define = optcmp(str);
	if(ret_function_define >= 0) {
		char function_name[32];
		char *str_bak = str;
		int v_len = len;
		node_attribute_t node_ptr;
		int function_declare_flag = 0;
		while(v_len > 0) {
			int token_len;
			token_len = get_token(str_bak, &node_ptr);
			if(node_ptr.node_type == TOKEN_NAME) {
				kstrcpy(function_name, node_ptr.value.ptr_value);
				get_token(str_bak + token_len, &node_ptr);
				if(node_ptr.node_type == TOKEN_OPERATOR && node_ptr.value.int_value == OPT_L_SMALL_BRACKET) {
					str = str_bak;
					function_declare_flag = 1;
					break;
				}
			}
			v_len -= token_len;
			str_bak += token_len;
		}
		int l_bracket_pos = str_find(str, '(');
		int r_bracket_pos = str_find(str, ')');
		if(function_declare_flag) {
			int symbol_begin_pos, ptr_level = 0;
			int keylen = kstrlen(type_key[ret_function_define]);
			stack* arg_stack;
			this->function_flag_set.function_flag = 1;
			this->function_flag_set.function_begin_flag = 1;
			int i=keylen+1;
			symbol_begin_pos=(str[keylen]==' '?i:i-1);
			//for(int j=symbol_begin_pos; str[j]=='*'; j++)
			//	ptr_level++;
			//symbol_begin_pos += ptr_level;
			//for(i=symbol_begin_pos; str[i]!='('; i++);
			//kmemcpy(function_name, str + symbol_begin_pos, i - symbol_begin_pos);
			//function_name[i - symbol_begin_pos] = 0;
			varity_info* arg_node_ptr = (varity_info*)vmalloc(sizeof(varity_info) * MAX_FUNCTION_ARGC);
			arg_stack = (stack*)vmalloc(sizeof(stack));
			arg_stack->init(sizeof(varity_info), arg_node_ptr, MAX_FUNCTION_ARGC);
			arg_node_ptr->arg_init("", ret_function_define, sizeof_type[ret_function_define], 0);//TODO:加上offset
			arg_stack->push(arg_node_ptr++);
			bool void_flag = false;
			int offset = 0;
			this->varity_global_flag = VARITY_SCOPE_LOCAL;
			for(int i=l_bracket_pos+1; i<r_bracket_pos; i++) {
				char varity_name[32];
				int type, arg_name_begin_pos, arg_name_end_pos;
				int pos = key_match(str + i, r_bracket_pos-i+1, &type);
				if(pos < 0) {
					error("arg type error.\n");
					ARG_RETURN(ERROR_FUNC_ARG_LIST);
				}
				keylen = kstrlen(type_key[type]);
				arg_name_begin_pos = i + pos + keylen + (str[i + pos + keylen] == ' ' ? 1 : 0);
				for(int j=arg_name_begin_pos; str[j]=='*'; j++)
					ptr_level++;
				if(type == VOID && !ptr_level) {
					if(arg_stack->get_count() > 1) {
						error("arg list error.\n");
						ARG_RETURN(ERROR_FUNC_ARG_LIST);
					}
					void_flag = true;
				} else {
					if(void_flag) {
						error("arg cannot use void type.\n");
						ARG_RETURN(ERROR_FUNC_ARG_LIST);
					}
				}
				arg_name_begin_pos += ptr_level;
				for(int j=arg_name_begin_pos; j<=r_bracket_pos; j++)
					if(str[j] == ',' || str[j] == ')') {
						arg_name_end_pos = j - 1;
						i = j;
						break;
					}
				kmemcpy(varity_name, str + arg_name_begin_pos, arg_name_end_pos - arg_name_begin_pos + 1);
				varity_name[arg_name_end_pos - arg_name_begin_pos + 1] = 0;
				if(!void_flag) {
					arg_node_ptr->arg_init(varity_name, type, sizeof_type[type], (void*)make_align(offset, PLATFORM_WORD_LEN));
					this->varity_declare->declare(VARITY_SCOPE_LOCAL, varity_name, type, sizeof_type[type], 0, 0);
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

bool c_interpreter::is_operator_convert(char *str, int &type, int &opt_len, int &prio)
{
	switch(type) {
	case OPT_PLUS:
	case OPT_MINUS:
		if(this->sentence_analysis_data_struct.last_token.node_type == TOKEN_OPERATOR 
			&& this->sentence_analysis_data_struct.last_token.value.int_value != OPT_R_SMALL_BRACKET 
			&& this->sentence_analysis_data_struct.last_token.value.int_value != OPT_L_MINUS_MINUS 
			&& this->sentence_analysis_data_struct.last_token.value.int_value != OPT_L_PLUS_PLUS
			&& this->sentence_analysis_data_struct.last_token.value.int_value != OPT_R_PLUS_PLUS
			&& this->sentence_analysis_data_struct.last_token.value.int_value != OPT_R_MINUS_MINUS) {
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
		next_token_len = this->get_token(str + opt_len, &next_node_attribute);
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
			&& this->sentence_analysis_data_struct.last_token.value.int_value != OPT_R_SMALL_BRACKET 
			&& this->sentence_analysis_data_struct.last_token.value.int_value != OPT_L_MINUS_MINUS 
			&& this->sentence_analysis_data_struct.last_token.value.int_value != OPT_L_PLUS_PLUS
			&& this->sentence_analysis_data_struct.last_token.value.int_value != OPT_R_PLUS_PLUS
			&& this->sentence_analysis_data_struct.last_token.value.int_value != OPT_R_MINUS_MINUS) {
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
		if(node.node_type == TOKEN_OPERATOR && node.value.int_value == OPT_TERNARY_C) {
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
	}
	return ERROR_NO;
}

int c_interpreter::generate_mid_code(char *str, uint len, bool need_semicolon)//TODO:所有uint len统统改int，否则传个-1进来
{
	if(len == 0 || str[0] == '{' || str[0] == '}')return ERROR_NO;
	sentence_analysis_data_struct_t *analysis_data_struct_ptr = &this->sentence_analysis_data_struct;
	int token_len;
	int node_index = 0;
	node_attribute_t *node_attribute, *stack_top_node_ptr;
	this->sentence_analysis_data_struct.last_token.node_type = TOKEN_OPERATOR;
	this->sentence_analysis_data_struct.last_token.value.int_value = OPT_EDGE;
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
			int read_seq_depth = this->nonseq_info->non_seq_struct_depth > this->nonseq_info->row_info_node[this->nonseq_info->row_num - 1].non_seq_depth ? this->nonseq_info->non_seq_struct_depth : this->nonseq_info->row_info_node[this->nonseq_info->row_num - 1].non_seq_depth;
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
			int read_seq_depth = this->nonseq_info->non_seq_struct_depth > this->nonseq_info->row_info_node[this->nonseq_info->row_num - 1].non_seq_depth ? this->nonseq_info->non_seq_struct_depth : this->nonseq_info->row_info_node[this->nonseq_info->row_num - 1].non_seq_depth;
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
		token_len = this->get_token(str, node_attribute);
		if(len < token_len)
			break;
		if(node_attribute->node_type == TOKEN_ERROR)
			return node_attribute->node_type;
		if(node_attribute->node_type == TOKEN_OPERATOR) {
			while(1) {
				is_operator_convert(str, node_attribute->value.int_value, token_len, node_attribute->value_type);
				stack_top_node_ptr = (node_attribute_t*)analysis_data_struct_ptr->expression_tmp_stack.get_lastest_element()->value;
				if(!analysis_data_struct_ptr->expression_tmp_stack.get_count() 
					|| node_attribute->value_type < stack_top_node_ptr->value_type 
					|| node_attribute->value.int_value == OPT_L_SMALL_BRACKET 
					|| stack_top_node_ptr->value.int_value == OPT_L_SMALL_BRACKET
					|| (node_attribute->value_type == stack_top_node_ptr->value_type && (node_attribute->value_type == 2 || node_attribute->value_type == 14))) {
					if(node_attribute->value.int_value == OPT_R_SMALL_BRACKET) {
						analysis_data_struct_ptr->expression_tmp_stack.pop();
						break;
					} else {
						if(node_attribute->value.int_value == OPT_CALL_FUNC) {
							analysis_data_struct_ptr->expression_tmp_stack.push(&analysis_data_struct_ptr->node_struct[node_index]);
							node_attribute = &analysis_data_struct_ptr->node_attribute[++node_index];
							analysis_data_struct_ptr->node_struct[node_index].value = node_attribute;
							node_attribute->node_type = TOKEN_OPERATOR;
							node_attribute->value_type = 1;
							node_attribute->value.int_value = OPT_L_SMALL_BRACKET;
						} else if(node_attribute->value.int_value == OPT_PLUS_PLUS || node_attribute->value.int_value == OPT_MINUS_MINUS) {
							node_attribute_t *final_stack_top_ptr = (node_attribute_t*)analysis_data_struct_ptr->expression_final_stack.get_lastest_element()->value;
							if(!analysis_data_struct_ptr->expression_final_stack.get_count() || this->sentence_analysis_data_struct.last_token.node_type == TOKEN_OPERATOR) {
								if(node_attribute->value.int_value == OPT_PLUS_PLUS)
									node_attribute->value.int_value = OPT_L_PLUS_PLUS;
								else
									node_attribute->value.int_value = OPT_L_MINUS_MINUS;
							} else {
								if(node_attribute->value.int_value == OPT_PLUS_PLUS)
									node_attribute->value.int_value = OPT_R_PLUS_PLUS;
								else
									node_attribute->value.int_value = OPT_R_MINUS_MINUS;
							}
						} else if(node_attribute->value.int_value == OPT_COMMA) {
							for(int n=node_index-1; n>=0; n--) {
								if(analysis_data_struct_ptr->node_attribute[n].node_type == TOKEN_OPERATOR && analysis_data_struct_ptr->node_attribute[n].value.int_value == OPT_L_SMALL_BRACKET) {
									if(n>0 && analysis_data_struct_ptr->node_attribute[n-1].node_type == TOKEN_OPERATOR && analysis_data_struct_ptr->node_attribute[n-1].value.int_value == OPT_CALL_FUNC) {
										node_attribute->value.int_value = OPT_FUNC_COMMA;
										break;
									}
								}
							}
						} else if(node_attribute->value.int_value == OPT_INDEX) {
							analysis_data_struct_ptr->expression_tmp_stack.push(&analysis_data_struct_ptr->node_struct[node_index]);
							node_attribute = &analysis_data_struct_ptr->node_attribute[++node_index];
							analysis_data_struct_ptr->node_struct[node_index].value = node_attribute;
							node_attribute->node_type = TOKEN_OPERATOR;
							node_attribute->value_type = 1;
							node_attribute->value.int_value = OPT_L_SMALL_BRACKET;
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
		if(((node_attribute_t*)last_token->value)->node_type != TOKEN_OPERATOR || ((node_attribute_t*)last_token->value)->value.int_value != OPT_EDGE) {
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
	//		printf("%d %d\n",tmp->value.int_value,tmp->value_type);
	//}
	node *root = analysis_data_struct_ptr->expression_final_stack.pop();
	if(!root) {
		error("No token found.\n");
		return 0;//TODO:找个合适的返回值
	}
	this->sentence_analysis_data_struct.tree_root = root;
	int ret;
	if(analysis_data_struct_ptr->expression_final_stack.get_count() == 0) {
		ret = generate_expression_value(this->cur_mid_code_stack_ptr, (node_attribute_t*)root->value);
		if(ret)return ret;
		//return ERROR_NO;
	} else {
		root->link_reset();
		ret = list_stack_to_tree(root, &analysis_data_struct_ptr->expression_final_stack);//二叉树完成
		if(ret)return ret;
		//root->middle_visit();
		ret = this->tree_to_code(root, this->cur_mid_code_stack_ptr);//构造中间代码
		if(ret)return ret;
		if(this->sentence_analysis_data_struct.short_depth) {
			error("? && : unmatch.\n");
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
						error("Varity not exist\n");
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

int c_interpreter::exec_mid_code(mid_code *pc, uint count)
{
	int ret;
	mid_code *end_ptr = pc + count;
	this->pc = pc;
	while(this->pc < end_ptr) {
		ret = call_opt_handle(this);
		if(ret) return ret;
		this->pc++;
	}
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
	debug("nonseqret=%d\n", ret2);
	//if(ret2 == OK_NONSEQ_FINISH || ret2 == OK_NONSEQ_INPUTING || ret2 == OK_NONSEQ_DEFINE) {
	//	if(this->cur_mid_code_stack_ptr == &this->mid_code_stack) {
	//		this->varity_global_flag = VARITY_SCOPE_LOCAL;
	//	}
	//	if(nonseq_info->row_info_node[nonseq_info->row_num].non_seq_depth) {
	//		switch(nonseq_info->row_info_node[nonseq_info->row_num].non_seq_info) {
	//		case 0:
	//			this->nonseq_start_gen_mid_code(str, len, nonseq_info->row_info_node[nonseq_info->row_num].nonseq_type);
	//			break;
	//		case 1:
	//			this->nonseq_end_gen_mid_code(nonseq_info->row_info_node[nonseq_info->row_num].row_ptr, nonseq_info->row_info_node[nonseq_info->row_num].row_len);
	//			break;
	//		case 2:
	//			this->nonseq_mid_gen_mid_code(str, len);
	//			break;
	//		default:
	//			break;
	//		}
	//	} else {
	//		this->sentence_exec(str, len, true, 0);
	//	}
	//	if(this->cur_mid_code_stack_ptr == &this->mid_code_stack && ret2 == OK_NONSEQ_FINISH) {
	//		this->varity_global_flag = VARITY_SCOPE_GLOBAL;
	//		this->nonseq_info->stack_frame_size = this->varity_declare->local_varity_stack->offset;
	//		this->varity_declare->destroy_local_varity_cur_depth();
	//	}
	//} else if(ret2 == ERROR_NONSEQ_GRAMMER)
	//	return ret2;
	
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
		ret1 = sentence_exec(str, len, true, NULL);
		return ret1;
	}
	return ERROR_NO;
}

int c_interpreter::nonseq_mid_gen_mid_code(char *str, uint len)
{
	mid_code* mid_code_ptr;
	int cur_depth = nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth;
	mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
	mid_code_ptr->ret_operator = CTL_BRANCH;
	this->cur_mid_code_stack_ptr->push();
	nonseq_info->row_info_node[nonseq_info->row_num - 1].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
	for(int i=nonseq_info->row_num-1; i>=0; i--) {
		if(nonseq_info->row_info_node[i].non_seq_depth == cur_depth 
			&& nonseq_info->row_info_node[i].non_seq_info == 0) {
			mid_code_ptr = nonseq_info->row_info_node[i].post_info_b + (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
			mid_code_ptr->opda_addr++;
			nonseq_info->row_info_node[i].finish_flag = 1;
			break;
		}
	}
	return ERROR_NO;
}

int c_interpreter::nonseq_end_gen_mid_code(char *str, uint len)
{
	int i, key_len;
	int cur_depth = nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth;
	if(len == 0)return ERROR_NO;
	mid_code *mid_code_ptr;
	node_attribute_t cur_node;
	key_len = get_token(str, &cur_node);
	if(cur_node.node_type != TOKEN_KEYWORD_NONSEQ && str[0] != '}') {//TODO：do while 检测bug
		if(!nonseq_info->row_info_node[nonseq_info->row_num - 1].finish_flag) {
			nonseq_info->row_info_node[nonseq_info->row_num - 1].finish_flag = 1;
			int ret = this->generate_mid_code(str, len, true);
			if(ret) return ret;
		}
	}
	for(i=nonseq_info->row_num-1; i>=0; i--) {
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
	mid_code* mid_code_ptr;
	node_attribute_t token_node;
	int key_len = get_token(str, &token_node), first_flag_pos, second_flag_pos;
	int begin_pos = get_token(str + key_len, &token_node);
	if((token_node.node_type != TOKEN_OPERATOR || token_node.value.int_value != OPT_L_SMALL_BRACKET) && token_node.value.int_value != NONSEQ_KEY_DO) {
		error("Lack of bracket\n");
		return ERROR_NONSEQ_GRAMMER;
	}
	switch(non_seq_type) {
	case NONSEQ_KEY_IF:
		this->generate_mid_code(str + key_len, len - key_len, false);
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num - 1].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
		mid_code_ptr->ret_operator = CTL_BRANCH_FALSE;
		this->cur_mid_code_stack_ptr->push();
		break;
	case NONSEQ_KEY_FOR: 
		first_flag_pos = find_ch_with_bracket_level(str + key_len, ';', 1);
		second_flag_pos = find_ch_with_bracket_level(str + key_len + first_flag_pos + 1, ';', 0);
		nonseq_info->row_info_node[nonseq_info->row_num - 1].post_info_c = key_len + first_flag_pos + second_flag_pos + 1;
		if(first_flag_pos == -1 || second_flag_pos == -1)
			return ERROR_NONSEQ_GRAMMER;
		this->generate_mid_code(str + key_len + begin_pos, first_flag_pos - begin_pos, false);
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num - 1].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();//循环条件判断处中间代码地址
		this->generate_mid_code(str + key_len + first_flag_pos + 1, second_flag_pos, false);
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num - 1].post_info_a = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr() - nonseq_info->row_info_node[nonseq_info->row_num - 1].post_info_b;//循环体处距循环条件中间代码处相对偏移
		mid_code_ptr->ret_operator = CTL_BRANCH_FALSE;
		this->cur_mid_code_stack_ptr->push();
		break;
	case NONSEQ_KEY_WHILE:
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num - 1].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
		this->generate_mid_code(str + key_len, len - key_len, false);
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num - 1].post_info_a = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr() - nonseq_info->row_info_node[nonseq_info->row_num - 1].post_info_b;//循环体处距循环条件中间代码处相对偏移
		mid_code_ptr->ret_operator = CTL_BRANCH_FALSE;
		this->cur_mid_code_stack_ptr->push();
		break;
	case NONSEQ_KEY_DO:
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num - 1].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
		break;
	}
	return ERROR_NO;
}

int c_interpreter::struct_analysis(char* str, uint len)
{
	int is_varity_declare = optcmp(str);
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
				//this->struct_declare->save_sentence(str, len);
				char varity_name[32];
				stack *varity_stack_ptr = this->struct_declare->current_node->varity_stack_ptr;
				varity_info* new_node_ptr = (varity_info*)varity_stack_ptr->get_current_ptr();
				int varity_type, varity_name_begin_pos, ptr_level = 0, key_len;
				varity_type = optcmp(str);
				key_len = kstrlen(type_key[varity_type]);
				varity_name_begin_pos = key_len + (str[key_len] == ' ' ? 1 : 0); 
				int opt_len, opt_type, symbol_pos_once, symbol_pos_last = str[key_len]==' '?key_len+1:key_len, size = len - symbol_pos_last;
				int array_flag = 0, element_count = 1;
				while(1) {
					for(ptr_level=0; str[symbol_pos_last]=='*'; symbol_pos_last++)
						ptr_level++;
					size -= ptr_level;
					varity_type += ptr_level * BASIC_VARITY_TYPE_COUNT;
					if((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) >= 0) {
						if(opt_type != OPT_COMMA && opt_type != OPT_EDGE && opt_type != OPT_L_MID_BRACKET && opt_type != OPT_R_MID_BRACKET) {
							error("Wrong operator exist in struct definition\n");
							return -1;
						} else if(opt_type == OPT_L_MID_BRACKET) {
							kmemcpy(varity_name, str + symbol_pos_last, symbol_pos_once);
							varity_name[symbol_pos_once] = 0;
							array_flag = 1;
						} else if(opt_type == OPT_R_MID_BRACKET) {
							array_flag = 2;
							element_count = y_atoi(str + symbol_pos_last, symbol_pos_once);
						} else {
							if(array_flag == 0) {
								kmemcpy(varity_name, str + symbol_pos_last, symbol_pos_once);
								varity_name[symbol_pos_once] = 0;
							} else if(array_flag == 2) {

							} else {
								
							}
							int varity_size;
							if(varity_type < BASIC_VARITY_TYPE_COUNT)
								varity_size = sizeof_type[varity_type];
							else
								varity_size = PLATFORM_WORD_LEN;
							this->struct_info_set.current_offset = make_align(this->struct_info_set.current_offset, varity_size);
							new_node_ptr->arg_init(varity_name, varity_type, varity_size * element_count, (void*)this->struct_info_set.current_offset);
							this->struct_info_set.current_offset += varity_size * element_count;
							varity_stack_ptr->push();
							array_flag = 0;
							element_count = 1;
							ptr_level = 0;
						}
					} else {
						break;
					}
					symbol_pos_last += symbol_pos_once + opt_len;
					size -= symbol_pos_once + opt_len;
				}
				/*for(int j=varity_name_begin_pos; str[j]=='*'; j++)
					ptr_level++;
				varity_name_begin_pos += ptr_level;

				kmemcpy(varity_name, str + varity_name_begin_pos, len - varity_name_begin_pos + 1);
				varity_name[len - varity_name_begin_pos - 1] = 0;
				new_node_ptr->arg_init(varity_name, varity_type, sizeof_type[varity_type], (void*)this->struct_info_set.current_offset);
				this->struct_info_set.current_offset = make_align(this->struct_info_set.current_offset, sizeof_type[varity_type]) + sizeof_type[varity_type];
				varity_stack_ptr->push();*/

				return OK_STRUCT_INPUTING;
			}
		}
	}
	if(is_varity_declare == STRUCT) {
		int space_count = char_count(str + 6, ' ');
		if(space_count == 1) {//1:define 2+:declare
			int symbol_begin_pos, ptr_level = 0;
			int keylen = kstrlen(type_key[is_varity_declare]);
			stack* arg_stack;
			char struct_name[32];
			this->struct_info_set.declare_flag = 1;
			this->struct_info_set.struct_begin_flag = 1;
			this->struct_info_set.current_offset = 0;
			int i = keylen + 1;
			symbol_begin_pos = i;
			for(int j=symbol_begin_pos; str[j]=='*'; j++)
				ptr_level++;
			symbol_begin_pos += ptr_level;
			kmemcpy(struct_name, str + symbol_begin_pos, len - symbol_begin_pos);
			struct_name[len - symbol_begin_pos] = 0;
			varity_attribute* arg_node_ptr = (varity_attribute*)vmalloc(sizeof(varity_info) * MAX_VARITY_COUNT_IN_STRUCT);
			arg_stack = (stack*)vmalloc(sizeof(stack));
			stack tmp_stack(sizeof(varity_info), arg_node_ptr, MAX_VARITY_COUNT_IN_STRUCT);
			kmemcpy(arg_stack, &tmp_stack, sizeof(stack));
			//arg_stack->init(sizeof(varity_info), arg_node_ptr, MAX_VARITY_COUNT_IN_STRUCT);
			this->struct_declare->declare(struct_name, arg_stack);
			return OK_STRUCT_INPUTING;
		}
	}
	return OK_STRUCT_NOSTRUCT;
}

int c_interpreter::key_word_analysis(char* str, uint len)
{
	int is_varity_declare;
	is_varity_declare = optcmp(str);
	char struct_name[32];
	struct_info* struct_node_ptr = 0;
	if(is_varity_declare >= 0) {
		int key_len = kstrlen(type_key[is_varity_declare]);
		if(is_varity_declare != STRUCT)
			len = remove_char(str + key_len + 1, ' ') + key_len + 1;
		else {//TODO: 处理结构名后接*的情况
			int space_2nd_pos = str_find(str + key_len + 1, len - key_len - 1, ' ') + key_len + 1;
			if(str[space_2nd_pos - 1] == '*')
				space_2nd_pos--;
			kmemcpy(struct_name, str + key_len + 1, space_2nd_pos - key_len - 1);
			struct_name[space_2nd_pos - key_len - 1] = 0;
			struct_node_ptr = this->struct_declare->find(struct_name);
			if(!struct_node_ptr) {
				error("There is no struct called %s.\n", struct_name);
				return ERROR_STRUCT_NONEXIST;
			}
			len = remove_char(str + space_2nd_pos + 1, ' ') + space_2nd_pos + 1;
			key_len = space_2nd_pos;
		}
		str += key_len;
		//解析复杂变量类型///////////////
		char varity_name[32];
		node_attribute_t *node_attribute, *stack_top_node_ptr;
		sentence_analysis_data_struct_t *analysis_data_struct_ptr = &this->sentence_analysis_data_struct;
		int node_index = 0, token_len, v_len = len - key_len;
		int token_flag = 0, array_flag = 0, ptr_level = 0;
		int varity_basic_type = 0, varity_size, ret;
		varity_info *new_varity_ptr;
		list_stack *cur_stack_ptr = &analysis_data_struct_ptr->expression_tmp_stack;
		this->sentence_analysis_data_struct.last_token.node_type = TOKEN_OPERATOR;
		this->sentence_analysis_data_struct.last_token.value.int_value = OPT_EDGE;
		while(v_len > 0) {
			node_attribute = &analysis_data_struct_ptr->node_attribute[node_index];
			analysis_data_struct_ptr->node_struct[node_index].value = node_attribute;
			token_len = this->get_token(str, node_attribute);
			//if(v_len < token_len) //error
			//	break;
			if(node_attribute->node_type == TOKEN_ERROR)
				return node_attribute->node_type;//TODO:找一个合适的返回值，generate_mid_code也一样
			if(node_attribute->node_type == TOKEN_OPERATOR) {
				stack_top_node_ptr = (node_attribute_t*)analysis_data_struct_ptr->expression_tmp_stack.get_lastest_element()->value;
				if(node_attribute->value.int_value == OPT_L_SMALL_BRACKET) {
					analysis_data_struct_ptr->expression_tmp_stack.push(&analysis_data_struct_ptr->node_struct[node_index]);
				} else if(node_attribute->value.int_value == OPT_R_SMALL_BRACKET) {
					while(1) {
						stack_top_node_ptr = (node_attribute_t*)analysis_data_struct_ptr->expression_tmp_stack.get_lastest_element()->value;
						if(stack_top_node_ptr->node_type == TOKEN_OPERATOR && stack_top_node_ptr->value.int_value == OPT_L_SMALL_BRACKET)
							break;
						analysis_data_struct_ptr->expression_final_stack.push(analysis_data_struct_ptr->expression_tmp_stack.pop());
					}
					analysis_data_struct_ptr->expression_tmp_stack.pop();
				} else if(node_attribute->value.int_value == OPT_L_MID_BRACKET) {
					array_flag = 1;
				} else if(node_attribute->value.int_value == OPT_R_MID_BRACKET) {
					array_flag = 0;
					cur_stack_ptr->push(&analysis_data_struct_ptr->node_struct[node_index]);
					node_attribute->value.int_value = OPT_INDEX;
					node_attribute->value_type = this->sentence_analysis_data_struct.last_token.value.int_value;
				} else if(node_attribute->value.int_value == OPT_MUL) {
					static node_attribute_t *ptr_node_ptr;
					if(this->sentence_analysis_data_struct.last_token.node_type == TOKEN_OPERATOR && this->sentence_analysis_data_struct.last_token.value.int_value == OPT_PTR_CONTENT) {
						ptr_node_ptr->value_type++;
					} else {
						cur_stack_ptr->push(&analysis_data_struct_ptr->node_struct[node_index]);
						node_attribute->value.int_value = OPT_PTR_CONTENT;
						node_attribute->value_type = 1;
						ptr_node_ptr = node_attribute;
					}
				} else if(node_attribute->value.int_value == OPT_COMMA || node_attribute->value.int_value == OPT_EDGE || node_attribute->value.int_value == OPT_ASSIGN) {
					while(stack_top_node_ptr = (node_attribute_t*)analysis_data_struct_ptr->expression_tmp_stack.get_lastest_element()->value) {
						analysis_data_struct_ptr->expression_final_stack.push(analysis_data_struct_ptr->expression_tmp_stack.pop());
					}
					int node_count = analysis_data_struct_ptr->expression_final_stack.get_count();
					uint *complex_info = 0, *cur_complex_info_ptr;
					int basic_info_node_count = 1;
					if(is_varity_declare == STRUCT)
						basic_info_node_count++;
					if(node_count) {
						if(ptr_level)
							node_count++;
						complex_info = (uint*)vmalloc((node_count + 1 + basic_info_node_count) * sizeof(int)); //+基本类型
						cur_complex_info_ptr = complex_info + node_count + basic_info_node_count;
						node *head = analysis_data_struct_ptr->expression_final_stack.get_head();
						for(int n=0; n<node_count; n++) {
							head = head->right;
							node_attribute_t *complex_attribute = (node_attribute_t*)head->value;
							if(complex_attribute->value.int_value == OPT_INDEX) {
								*cur_complex_info_ptr = (COMPLEX_ARRAY << COMPLEX_TYPE_BIT) | complex_attribute->value_type;
							} else if(complex_attribute->value.int_value == OPT_PTR_CONTENT) {
								*cur_complex_info_ptr = (COMPLEX_PTR << COMPLEX_TYPE_BIT) | complex_attribute->value_type;
							}
							cur_complex_info_ptr--;
						}
						if(ptr_level)
							*cur_complex_info_ptr-- = (COMPLEX_PTR << COMPLEX_TYPE_BIT) | ptr_level;
						*cur_complex_info_ptr-- = (COMPLEX_BASIC << COMPLEX_TYPE_BIT) | is_varity_declare;
						if(is_varity_declare == STRUCT) {
							*cur_complex_info_ptr-- = struct_node_ptr->type_info_ptr[1];//TODO:或许有更好办法
						}
						analysis_data_struct_ptr->expression_final_stack.reset();//避免后续声明错误及影响生成后序表达式
					}
					////////
					varity_basic_type = is_varity_declare + ptr_level * BASIC_VARITY_TYPE_COUNT;
					if(node_count)
						varity_basic_type = COMPLEX;
					else
						complex_info = (uint*)get_basic_info(varity_basic_type, struct_node_ptr, this->struct_declare);
					varity_size = get_varity_size(varity_basic_type, complex_info, node_count + basic_info_node_count);
					if(this->varity_global_flag == VARITY_SCOPE_GLOBAL) {
						ret = this->varity_declare->declare(VARITY_SCOPE_GLOBAL, varity_name, varity_basic_type, varity_size, node_count + basic_info_node_count, complex_info);
						new_varity_ptr = (varity_info*)this->varity_declare->global_varity_stack->get_lastest_element();
					} else {
						ret = this->varity_declare->declare(VARITY_SCOPE_LOCAL, varity_name, varity_basic_type, varity_size, node_count + basic_info_node_count, complex_info);
						new_varity_ptr = (varity_info*)this->varity_declare->local_varity_stack->get_lastest_element();
					}
					if(node_count) {
						//new_varity_ptr->config_complex_info(node_count + 1, complex_info);
						complex_info[0] = 1;//起始引用次数1
					}
					////////
					if(node_attribute->value.int_value == OPT_ASSIGN) {
						int exp_len = find_ch_with_bracket_level(str + 1, ',', 0);
						if(exp_len == -1)
							exp_len = find_ch_with_bracket_level(str + 1, ';', 0);
						generate_mid_code(str + 1, exp_len, false);
						mid_code *code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr() - 1;
						code_ptr->ret_addr = (int)new_varity_ptr->get_content_ptr();//TODO:避免数组被赋值
						code_ptr->ret_operand_type = this->varity_global_flag;
						code_ptr->ret_varity_type = new_varity_ptr->get_type();
						code_ptr->data = new_varity_ptr->get_element_size();
						str += exp_len + 1;
						v_len -= exp_len + 1;
					}
					node_index = 0;
					this->sentence_analysis_data_struct.last_token.node_type = TOKEN_OPERATOR;
					this->sentence_analysis_data_struct.last_token.value.int_value = OPT_EDGE;
					v_len -= token_len;
					str += token_len;
					continue;
				} else {
#if !DYNAMIC_ARRAY_EN
					error("Not allowed to use dynamic array.\n");
#endif
				}
			} else if(node_attribute->node_type == TOKEN_NAME) {
				token_flag = 1;
				cur_stack_ptr = &analysis_data_struct_ptr->expression_final_stack;
				kstrcpy(varity_name, node_attribute->value.ptr_value);
			}
			this->sentence_analysis_data_struct.last_token = *node_attribute;
			cur_stack_ptr = &analysis_data_struct_ptr->expression_tmp_stack;
			v_len -= token_len;
			str += token_len;
			node_index++;
		}
		return OK_VARITY_DECLARE;
	}
	return ERROR_NO;
}

int c_interpreter::sentence_exec(char* str, uint len, bool need_semicolon, varity_info* expression_value)
{
	int ret;
	int total_bracket_depth;
	char ch_last = str[len];
	int source_len = len;
	str[source_len] = 0;
	if(str[0] == '{' || str[0] == '}')
		return 0;
	if(str[len-1] != ';' && need_semicolon) {
		error("Missing ;\n");
		str[source_len] = ch_last;
		return ERROR_SEMICOLON;
	}
	total_bracket_depth = get_bracket_depth(str);
	if(total_bracket_depth < 0)
		return ERROR_BRACKET_UNMATCH;
	int key_word_ret = key_word_analysis(str, len);
	if(key_word_ret) {
		str[source_len] = ch_last;
		//return key_word_ret;
	} else {
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
			for(int i=0; i<g_varity_list.get_count(); i++) {
				if(g_varity_node[i].get_content_ptr() == (void*)ptr->ret_addr) {
					debug("%s=",g_varity_node[i].get_name());
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
			for(int i=0; i<g_varity_list.get_count(); i++) {
				if(g_varity_node[i].get_content_ptr() == (void*)ptr->opda_addr) {
					debug("%s ",g_varity_node[i].get_name());
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
			for(int i=0; i<g_varity_list.get_count(); i++) {
				if(g_varity_node[i].get_content_ptr() == (void*)ptr->opdb_addr) {
					debug("%s",g_varity_node[i].get_name());
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
	heapinit();
	myinterpreter.run_interpreter();
}