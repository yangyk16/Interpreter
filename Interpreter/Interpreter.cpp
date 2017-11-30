#include "interpreter.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "operator.h"
#include "string_lib.h"
#include "error.h"

tty stdio;
varity_info g_varity_node[MAX_G_VARITY_NODE];
varity_info l_varity_node[MAX_L_VARITY_NODE];
varity_info a_varity_node[MAX_A_VARITY_NODE];
stack a_varity_list(sizeof(varity_info), a_varity_node, MAX_A_VARITY_NODE);
indexed_stack l_varity_list(sizeof(varity_info), l_varity_node, MAX_L_VARITY_NODE);
stack g_varity_list(sizeof(varity_info), g_varity_node, MAX_G_VARITY_NODE);
varity c_varity(&g_varity_list, &l_varity_list, &a_varity_list);
nonseq_info_struct nonseq_info_s;
function_info function_node[MAX_FUNCTION_NODE];
stack function_list(sizeof(function_info), function_node, MAX_FUNCTION_NODE);
function c_function(&function_list);
struct_info struct_node[MAX_STRUCT_NODE];
stack struct_list(sizeof(struct_info), struct_node, MAX_STRUCT_NODE);
struct_define c_struct(&struct_list);
c_interpreter myinterpreter(&stdio, &c_varity, &nonseq_info_s, &c_function, &c_struct);

char non_seq_key[][7] = {"", "if", "switch", "else", "for", "while", "do"};
const char non_seq_key_len[] = {0, 2, 6, 4, 3, 5, 2};
char opt_str[43][4] = {"<<=",">>=","->","++","--","<<",">>",">=","<=","==","!=","&&","||","/=","*=","%=","+=","-=","&=","^=","|=","[","]","(",")",".","-","~","*","&","!","/","%","+",">","<","^","|","?",":","=",",",";"};
const char opt_str_len[] = {3,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
const char opt_prio[] ={14,14,1,2,2,5,5,6,6,7,7,11,12,14,14,14,14,14,14,14,14,1,1,1,17,1,4,2,3,8,2,3,3,4,6,6,9,10,13,13,14,15,16};
const char opt_number[] = {2,2,2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,2,2,2,1,1,1,1};
char tmp_varity_name[MAX_A_VARITY_NODE][3];

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
		} else if(last_node_attribute->node_type == TOKEN_NAME || last_node_attribute->node_type == TOKEN_CONST_VALUE) {
		} else {
			error("Invalid key word.\n");
			return ERROR_INVALID_KEYWORD;
		}
		if(opt_number[((node_attribute_t*)tree_node->value)->value.int_value] == 1) {
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
		} else if(last_node_attribute->node_type == TOKEN_NAME || last_node_attribute->node_type == TOKEN_CONST_VALUE) {
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

int c_interpreter::operator_post_handle(stack *code_stack_ptr, node *opt_node_ptr)
{
	register varity_info *varity_ptr;
	register int opt = ((node_attribute_t*)opt_node_ptr->value)->value.int_value;
	register int varity_scope;
	register node_attribute_t *node_attribute;
	register mid_code *instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
	int varity_number;
	if(opt_number[opt] > 1) {
		node_attribute = (node_attribute_t*)opt_node_ptr->left->value;//判断是否是双目，单目不能看左树
		if(node_attribute->node_type == TOKEN_CONST_VALUE) {
			instruction_ptr->opda_operand_type = OPERAND_CONST;
			instruction_ptr->opda_varity_type = node_attribute->value_type;
			memcpy(&instruction_ptr->opda_addr, &node_attribute->value, 8);
		} else if(node_attribute->node_type == TOKEN_NAME) {
			if(node_attribute->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {
				instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
				instruction_ptr->opda_addr = 8 * node_attribute->value.ptr_value[1];
				instruction_ptr->opda_varity_type = ((varity_info*)this->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]))->get_type();
			} else {
				if(opt != OPT_CALL_FUNC) {
					varity_ptr = this->varity_declare->vfind(node_attribute->value.ptr_value, varity_scope);
					if(!varity_ptr) {
						error("Varity not exist\n");
						return ERROR_VARITY_NONEXIST;
					}
					if(varity_scope == VARITY_SCOPE_GLOBAL)
						instruction_ptr->opda_operand_type = OPERAND_G_VARITY;
					else
						instruction_ptr->opda_operand_type = OPERAND_L_VARITY;
					instruction_ptr->opda_addr = (int)varity_ptr->get_content_ptr();
					instruction_ptr->opda_varity_type = varity_ptr->get_type();
				}
			}
		}
	}
	if(opt_node_ptr->right) {
		node_attribute = (node_attribute_t*)opt_node_ptr->right->value;
		if(node_attribute->node_type == TOKEN_CONST_VALUE) {
			instruction_ptr->opdb_operand_type = OPERAND_CONST;
			instruction_ptr->opdb_varity_type = node_attribute->value_type;
			memcpy(&instruction_ptr->opdb_addr, &node_attribute->value, 8);
		} else if(node_attribute->node_type == TOKEN_NAME) {
			if(node_attribute->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {
				instruction_ptr->opdb_operand_type = OPERAND_T_VARITY;
				instruction_ptr->opdb_addr = 8 * node_attribute->value.ptr_value[1];
				instruction_ptr->opdb_varity_type = ((varity_info*)this->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]))->get_type();
			} else {
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
	}

	switch(opt) {
		int ret_type;
	case OPT_PLUS:
	case OPT_MINUS:
	case OPT_MUL:
	case OPT_DIVIDE:
		ret_type = min(instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type);
		if(instruction_ptr->opda_operand_type == OPERAND_T_VARITY && instruction_ptr->opdb_operand_type == OPERAND_T_VARITY) {
			varity_number = ((node_attribute_t*)opt_node_ptr->left->value)->value.ptr_value[1];
			this->mid_varity_stack.pop();
		} else if(instruction_ptr->opda_operand_type == OPERAND_T_VARITY && instruction_ptr->opdb_operand_type != OPERAND_T_VARITY) {
			varity_number = ((node_attribute_t*)opt_node_ptr->left->value)->value.ptr_value[1];
		} else if(instruction_ptr->opda_operand_type != OPERAND_T_VARITY && instruction_ptr->opdb_operand_type == OPERAND_T_VARITY) {
			varity_number = ((node_attribute_t*)opt_node_ptr->right->value)->value.ptr_value[1];
		} else {
			varity_number = this->mid_varity_stack.get_count();
			this->mid_varity_stack.push();
		}
		varity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		varity_ptr->set_type(ret_type);
		instruction_ptr->ret_addr = varity_number * 8;
		instruction_ptr->ret_operator = opt;
		instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_varity_type = min(instruction_ptr->opda_varity_type, instruction_ptr->opdb_varity_type);
		((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
		((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
		break;
	case OPT_EQU:
	case OPT_NOT_EQU:
	case OPT_BIG_EQU:
	case OPT_SMALL_EQU:
	case OPT_BIG:
	case OPT_SMALL:
		ret_type = INT;
		if(instruction_ptr->opda_operand_type == OPERAND_T_VARITY && instruction_ptr->opdb_operand_type == OPERAND_T_VARITY) {
			varity_number = ((node_attribute_t*)opt_node_ptr->left->value)->value.ptr_value[1];
			this->mid_varity_stack.pop();
		} else if(instruction_ptr->opda_operand_type == OPERAND_T_VARITY && instruction_ptr->opdb_operand_type != OPERAND_T_VARITY) {
			varity_number = ((node_attribute_t*)opt_node_ptr->left->value)->value.ptr_value[1];
		} else if(instruction_ptr->opda_operand_type != OPERAND_T_VARITY && instruction_ptr->opdb_operand_type == OPERAND_T_VARITY) {
			varity_number = ((node_attribute_t*)opt_node_ptr->right->value)->value.ptr_value[1];
		} else {
			varity_number = this->mid_varity_stack.get_count();
			this->mid_varity_stack.push();
		}
		varity_ptr = (varity_info*)this->mid_varity_stack.visit_element_by_index(varity_number);
		varity_ptr->set_type(ret_type);
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
		if(instruction_ptr->opda_varity_type < U_LONG_LONG || instruction_ptr->opda_varity_type > CHAR
			|| instruction_ptr->opdb_varity_type < U_LONG_LONG || instruction_ptr->opdb_varity_type > CHAR) {
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
			this->mid_varity_stack.pop();
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
		break;
	case OPT_R_PLUS_PLUS:
	case OPT_R_MINUS_MINUS:
		break;
	case OPT_NOT:
	case OPT_BIT_REVERT:

	case OPT_POSITIVE:
	case OPT_NEGATIVE:
		break;
	case OPT_ADDRESS_OF:
		break;
	case OPT_PTR_CONTENT:
		break;
	case OPT_BIT_AND:
	case OPT_BIT_OR:
	case OPT_MOD:
	case OPT_ASL:
	case OPT_ASR:
		break;
	case OPT_CALL_FUNC:
	case OPT_FUNC_COMMA:
		if(opt_node_ptr->right) {
			node_attribute = (node_attribute_t*)opt_node_ptr->right->value;
			if(node_attribute->node_type == TOKEN_NAME ||  node_attribute->node_type == TOKEN_CONST_VALUE) {
				stack *arg_list_ptr = (stack*)this->call_func_info.function_ptr[this->call_func_info.function_depth - 1]->arg_list;
				instruction_ptr->ret_operator = OPT_PASS_PARA;
				instruction_ptr->opda_operand_type = OPERAND_L_S_VARITY;
				instruction_ptr->opda_varity_type = ((varity_info*)(arg_list_ptr->visit_element_by_index(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1])))->get_type();
				instruction_ptr->opda_addr = (int)((varity_info*)(arg_list_ptr->visit_element_by_index(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1]++)))->get_content_ptr();
				if(instruction_ptr->opdb_operand_type == OPERAND_T_VARITY) {
					this->mid_varity_stack.pop();
				}
			}
		}
		if(opt == OPT_CALL_FUNC) {
			if(opt_node_ptr->right)
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
			instruction_ptr->ret_operand_type = OPERAND_T_VARITY;
			((node_attribute_t*)opt_node_ptr->value)->node_type = TOKEN_NAME;
			((node_attribute_t*)opt_node_ptr->value)->value.ptr_value = tmp_varity_name[varity_number];
			this->mid_varity_stack.push();
			this->call_func_info.function_depth--;
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
	return ERROR_NO;
}

int c_interpreter::operator_mid_handle(stack *code_stack_ptr, node *opt_node_ptr)
{
	register mid_code *instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
	node_attribute_t *node_attribute = ((node_attribute_t*)opt_node_ptr->value);
	switch(node_attribute->value.int_value) {
	case OPT_CALL_FUNC:
		this->call_func_info.function_ptr[this->call_func_info.function_depth] = this->function_declare->find(((node_attribute_t*)opt_node_ptr->left->value)->value.ptr_value);
		if(!this->call_func_info.function_ptr[this->call_func_info.function_depth]) {
			error("Function not found.\n");
			return ERROR_VARITY_NONEXIST; //TODO: 找一个合适的错误码
		}
		this->call_func_info.arg_count[this->call_func_info.function_depth] = this->call_func_info.function_ptr[this->call_func_info.function_depth]->arg_list->get_count();
		this->call_func_info.cur_arg_number[this->call_func_info.function_depth] = 0;
		this->call_func_info.function_depth++;
		break;
	case OPT_FUNC_COMMA:
		if(((node_attribute_t*)opt_node_ptr->left->value)->node_type == TOKEN_NAME ||  ((node_attribute_t*)opt_node_ptr->left->value)->node_type == TOKEN_CONST_VALUE) {
			stack *arg_list_ptr = (stack*)this->call_func_info.function_ptr[this->call_func_info.function_depth - 1]->arg_list;
			instruction_ptr->ret_operator = OPT_PASS_PARA;
			instruction_ptr->ret_operand_type = OPERAND_L_S_VARITY;
			instruction_ptr->ret_varity_type = ((varity_info*)(arg_list_ptr->visit_element_by_index(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1])))->get_type();
			instruction_ptr->ret_addr = (int)((varity_info*)(arg_list_ptr->visit_element_by_index(this->call_func_info.cur_arg_number[this->call_func_info.function_depth - 1]++)))->get_content_ptr();
			node_attribute = (node_attribute_t*)opt_node_ptr->left->value;
			if(node_attribute->node_type == TOKEN_CONST_VALUE) {
				instruction_ptr->opdb_operand_type = OPERAND_CONST;
				instruction_ptr->opdb_varity_type = node_attribute->value_type;
				memcpy(&instruction_ptr->opdb_addr, &node_attribute->value, 8);
			} else if(node_attribute->node_type == TOKEN_NAME) {
				if(node_attribute->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {
					instruction_ptr->opdb_operand_type = OPERAND_T_VARITY;
					instruction_ptr->opdb_addr = 8 * node_attribute->value.ptr_value[1];
					instruction_ptr->opdb_varity_type = ((varity_info*)this->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]))->get_type();
				} else {
					varity_info *varity_ptr;
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
			code_stack_ptr->push();
		}
		break;
	case OPT_AND:
	case OPT_OR:
		break;
	}
}

int c_interpreter::tree_to_code(node *tree, stack *code_stack)
{
	register int ret;
	if(tree->left && ((node_attribute_t*)tree->left->value)->node_type == TOKEN_OPERATOR)
		this->tree_to_code(tree->left, code_stack);
	//需要中序处理的几个运算符：CALL_FUNC，&&，||，FUNC_COMMA等
	if(((node_attribute_t*)tree->value)->node_type == TOKEN_OPERATOR && (((node_attribute_t*)tree->value)->value.int_value == OPT_CALL_FUNC
		|| ((node_attribute_t*)tree->value)->value.int_value == OPT_FUNC_COMMA
		|| ((node_attribute_t*)tree->value)->value.int_value == OPT_AND
		|| ((node_attribute_t*)tree->value)->value.int_value == OPT_OR)) {
		ret = this->operator_mid_handle(code_stack, tree);
	}
	if(tree->right && ((node_attribute_t*)tree->right->value)->node_type == TOKEN_OPERATOR)
		this->tree_to_code(tree->right, code_stack);
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
			if(!strcmp(symbol_ptr, type_key[j])) {
				info->value.int_value = j;
				info->node_type = TOKEN_KEYWORD_TYPE;
				return i;
			}
		}
		for(int j=0; j<sizeof(non_seq_key)/sizeof(non_seq_key[0]); j++) {
			if(!strcmp(symbol_ptr, type_key[j])) {
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
	while(1) {
		uint len;
		printf(">> ");
		len = this->row_pretreat_fifo.readline(sentence_buf);
		if(len > 0) {

		} else {
			tty_used->readline(sentence_buf);
			len = pre_treat();
		}
		this->sentence_analysis(sentence_buf, len);
	}
}

c_interpreter::c_interpreter(terminal* tty_used, varity* varity_declare, nonseq_info_struct* nonseq_info, function* function_declare, struct_define* struct_declare)
{
	this->tty_used = tty_used;
	this->varity_declare = varity_declare;
	this->nonseq_info = nonseq_info;
	this->function_declare = function_declare;
	this->struct_declare = struct_declare;
	this->function_return_value = (varity_info*)vmalloc(sizeof(varity_info));
	this->row_pretreat_fifo.set_base(this->pretreat_buf);
	this->row_pretreat_fifo.set_length(sizeof(this->pretreat_buf));
	this->non_seq_code_fifo.set_base(this->non_seq_tmp_buf);
	this->non_seq_code_fifo.set_length(sizeof(this->non_seq_tmp_buf));
	this->non_seq_code_fifo.set_element_size(1);
	this->token_fifo.init(MAX_ANALYSIS_BUFLEN);
	this->analysis_buf_ptr = this->analysis_buf;
	this->nonseq_info->nonseq_begin_stack_ptr = 0;
	this->call_func_info.function_depth = 0;
	this->varity_global_flag = VARITY_SCOPE_GLOBAL;
	///////////
	handle_init();
	this->stack_pointer = this->simulation_stack;
	this->tmp_varity_stack_pointer = this->tmp_varity_stack;
	this->mid_code_stack.init(sizeof(mid_code), MAX_MID_CODE_COUNT);
	this->mid_varity_stack.init(sizeof(varity_info), MAX_MID_CODE_COUNT);
	this->cur_mid_code_stack_ptr = &this->mid_code_stack;
	this->exec_flag = true;
	for(int i=0; i<MAX_A_VARITY_NODE; i++) {
		tmp_varity_name[i][0] = TMP_VAIRTY_PREFIX;
		tmp_varity_name[i][1] = i;
		tmp_varity_name[i][2] = 0;
	}
	///////////
	this->c_opt_caculate_func_list[0]=&c_interpreter::member_opt;
	this->c_opt_caculate_func_list[1]=&c_interpreter::auto_inc_opt;
	this->c_opt_caculate_func_list[2]=&c_interpreter::multiply_opt;
	this->c_opt_caculate_func_list[3]=&c_interpreter::plus_opt;
	this->c_opt_caculate_func_list[5]=&c_interpreter::relational_opt;
	this->c_opt_caculate_func_list[6]=&c_interpreter::equal_opt;
	this->c_opt_caculate_func_list[13]=&c_interpreter::assign_opt;
}

void nonseq_info_struct::reset(void)
{
	memset(this, 0, sizeof(nonseq_info_struct));
}

int c_interpreter::save_sentence(char* str, uint len)
{
	nonseq_info->row_info_node[nonseq_info->row_num].row_ptr = &non_seq_tmp_buf[non_seq_code_fifo.wptr];
	nonseq_info->row_info_node[nonseq_info->row_num].row_len = len;
	non_seq_code_fifo.write(str, len);
	non_seq_code_fifo.write("\0", 1);
	return 0;
}

int c_interpreter::non_seq_struct_analysis(char* str, uint len)
{
	nonseq_info->last_non_seq_check_ret = nonseq_info->non_seq_check_ret;
	if(len == 0) {
		if(nonseq_info->non_seq_struct_depth && nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] != NONSEQ_KEY_WAIT_ELSE) {
			error("blank shouldn't appear here.\n");
			nonseq_info->reset();
			return ERROR_NONSEQ_GRAMMER;
		}
	}
	nonseq_info->non_seq_check_ret = non_seq_struct_check(str);
	if(len && (nonseq_info->non_seq_check_ret || nonseq_info->non_seq_struct_depth)) {
		this->save_sentence(str, len);
		nonseq_info->row_num++;
	}
	if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_WAIT_ELSE) {
		if(nonseq_info->brace_depth == 0) {
			if(len != 0 && nonseq_info->non_seq_check_ret != NONSEQ_KEY_ELSE) {
				error("if is unmatch with else or blank\n");
				nonseq_info->reset();
				return ERROR_NONSEQ_GRAMMER;
			} else if(len == 0) {
				while(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] == 0 && this->nonseq_info->non_seq_struct_depth > 0) {
					nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth--]=0;
					if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_IF) {
						nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
						//break;
					} else if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_DO) {
						nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
						break;
					}
				}
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth = nonseq_info->non_seq_struct_depth + 1;
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 1;
				if(nonseq_info->non_seq_struct_depth == 0) {
					if(nonseq_info->non_seq_type_stack[0] != NONSEQ_KEY_WAIT_WHILE) {
						//save_sentence(str, len);
						nonseq_info->non_seq_exec = 1;
						return OK_NONSEQ_FINISH;
					} else {
						//save_sentence(str, len);
						return OK_NONSEQ_INPUTING;
					}
				}
			} else { //nonseq_info->non_seq_check_ret == NONSEQ_KEY_ELSE
				nonseq_info->row_info_node[nonseq_info->row_num - 2].non_seq_info = 3;
			}
		} else {
			if(nonseq_info->non_seq_check_ret != NONSEQ_KEY_ELSE && len != 0) {
				while(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] == 0 && this->nonseq_info->non_seq_struct_depth > 0) {
					//没清掉type_stack
					nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth--] = 0;
					if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_IF) {
						nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = 0;
						//nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
						//break;
					} else if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_DO) {
						nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
						break;
					}
				}
				nonseq_info->row_info_node[nonseq_info->row_num - 2].non_seq_depth = nonseq_info->non_seq_struct_depth + 1;
				nonseq_info->row_info_node[nonseq_info->row_num - 2].non_seq_info = 1;
			} else if(nonseq_info->non_seq_check_ret == NONSEQ_KEY_ELSE) {
				nonseq_info->row_info_node[nonseq_info->row_num - 2].non_seq_info = 3;
			}
		}
	} else if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_WAIT_WHILE) {
		if(nonseq_info->non_seq_check_ret != NONSEQ_KEY_WHILE && len != 0) {
			error("do is unmatch with while\n");
			nonseq_info->reset();
			return ERROR_NONSEQ_GRAMMER;
		} else if(nonseq_info->non_seq_check_ret == NONSEQ_KEY_WHILE) {
			nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WHILE;
			while(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] == 0 && this->nonseq_info->non_seq_struct_depth > 0) {
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth--]=0;
				if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_IF) {
					nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
					break;
				} else if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_DO) {
					nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
					break;
				}
			}
			if(nonseq_info->non_seq_struct_depth == 0 && nonseq_info->non_seq_type_stack[0] != NONSEQ_KEY_WAIT_WHILE && nonseq_info->non_seq_type_stack[0] != NONSEQ_KEY_WAIT_ELSE) {
				//save_sentence(str, len);
				nonseq_info->non_seq_exec = 1;
				return OK_NONSEQ_FINISH;
			} else {
				//save_sentence(str, len);
				return OK_NONSEQ_INPUTING;
			}
		}
	}
	if(nonseq_info->non_seq_check_ret) {
		nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 0;
		if(nonseq_info->non_seq_check_ret == NONSEQ_KEY_ELSE) {
			nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 2;
			if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] != NONSEQ_KEY_WAIT_ELSE || nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] != nonseq_info->brace_depth) {
				error("else is unmatch with if\n");
				nonseq_info->reset();
				return ERROR_NONSEQ_GRAMMER;
			}
			nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_IF;
		}
		nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = nonseq_info->non_seq_check_ret;
		nonseq_info->non_seq_struct_depth++;
		nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth = nonseq_info->non_seq_struct_depth;
		nonseq_info->row_info_node[nonseq_info->row_num - 1].nonseq_type = nonseq_info->non_seq_check_ret;
		return OK_NONSEQ_DEFINE;
	}
	if(len == 0)
		return OK_NONSEQ_INPUTING;
	if(nonseq_info->non_seq_struct_depth) {
		//save_sentence(str, len);
	}
	if(str[0] == '{' && nonseq_info->non_seq_struct_depth) {
		nonseq_info->brace_depth++;
		if(nonseq_info->last_non_seq_check_ret) {
			this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] = nonseq_info->brace_depth;
		}
	} else if(str[0] == '}') {
		if(nonseq_info->brace_depth > 0) {
			if(nonseq_info->non_seq_struct_depth > 0 && this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] == nonseq_info->brace_depth--) {
				do {
					nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth--]=0;
					if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_IF) {
						nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
						break;
					} else if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_DO) {
						nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
						break;
					}
				} while(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] == 0 && this->nonseq_info->non_seq_struct_depth > 0);
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 1;
				nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth = nonseq_info->non_seq_struct_depth + 1;
				if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_WAIT_ELSE) {
					//nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth++;
				}
				this->nonseq_info->nonseq_begin_stack_ptr--;
				if(nonseq_info->non_seq_struct_depth == 0)
					if(nonseq_info->non_seq_type_stack[0] == NONSEQ_KEY_WAIT_ELSE) {
						return OK_NONSEQ_INPUTING;
					} else if(nonseq_info->non_seq_type_stack[0] == NONSEQ_KEY_WAIT_WHILE) {
						return OK_NONSEQ_INPUTING;
					} else {
						nonseq_info->non_seq_exec = 1;
						return OK_NONSEQ_FINISH;
					}
			}
			//nonseq_info->brace_depth--;
		} else {
			error("there is no { to match\n");
			return ERROR_NONSEQ_GRAMMER;
		}
	}
	if(nonseq_info->last_non_seq_check_ret && nonseq_info->non_seq_struct_depth && str[len-1] == ';') {// && nonseq_info->brace_depth == 0
		do {
			nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth--]=0;
			if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_IF) {
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_ELSE;
				break;
			} else if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_DO) {
				nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] = NONSEQ_KEY_WAIT_WHILE;
				break;
			}
		} while(this->nonseq_info->nonseq_begin_bracket_stack[this->nonseq_info->non_seq_struct_depth] == 0 && this->nonseq_info->non_seq_struct_depth > 0);
		nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info = 1;
		nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth = nonseq_info->non_seq_struct_depth + 1;
		if(nonseq_info->non_seq_type_stack[nonseq_info->non_seq_struct_depth] == NONSEQ_KEY_WAIT_ELSE) {
			//nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth++;
		}
		if(nonseq_info->non_seq_struct_depth == 0) {
			if(nonseq_info->non_seq_type_stack[0] == NONSEQ_KEY_WAIT_ELSE) {
				return OK_NONSEQ_INPUTING;
			} else if(nonseq_info->non_seq_type_stack[0] == NONSEQ_KEY_WAIT_WHILE) {
				return OK_NONSEQ_INPUTING;
			} else {
				nonseq_info->non_seq_exec = 1;
				return OK_NONSEQ_FINISH;
			}
		}
	}
	if(nonseq_info->non_seq_struct_depth)
		return OK_NONSEQ_INPUTING;
	return ERROR_NO;
}

int c_interpreter::function_analysis(char* str, uint len)
{
	int ret_function_define;
	if(this->function_flag_set.function_flag) {
		this->function_declare->save_sentence(str, len);
		if(this->function_flag_set.function_begin_flag) {
			if(str[0] != '{' || len != 1) {
				return ERROR_FUNC_ERROR;
			}
			this->function_flag_set.function_begin_flag = 0;
		}
		if(str[0] == '{') {
			this->function_flag_set.brace_depth++;
		} else if(str[0] == '}') {
			this->function_flag_set.brace_depth--;
			if(!this->function_flag_set.brace_depth) {
				mid_code *mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr(), *code_end_ptr = mid_code_ptr;
				while(--mid_code_ptr >= (mid_code*)this->function_declare->get_current_node()->mid_code_stack.get_base_addr()) {
					if(mid_code_ptr->ret_operator == CTL_RETURN)
						mid_code_ptr->opda_addr = code_end_ptr - mid_code_ptr;
				}
				this->function_flag_set.function_flag = 0;
				this->cur_mid_code_stack_ptr = &this->mid_code_stack;
				this->function_declare->get_current_node()->stack_frame_size = this->varity_declare->local_varity_stack->offset;
				this->exec_flag = true;
				this->varity_global_flag = VARITY_SCOPE_GLOBAL;
				this->varity_declare->destroy_local_varity();
				return OK_FUNC_FINISH;
			}
		}
		return OK_FUNC_INPUTING;
	}
	ret_function_define = optcmp(str);
	if(ret_function_define >= 0) {
		int l_bracket_pos = str_find(str, '(');
		int r_bracket_pos = str_find(str, ')');
		if(l_bracket_pos > 0 && r_bracket_pos > l_bracket_pos) {
			int symbol_begin_pos, ptr_level = 0;
			int keylen = strlen(type_key[ret_function_define]);
			stack* arg_stack;
			char function_name[32];
			this->function_flag_set.function_flag = 1;
			this->function_flag_set.function_begin_flag = 1;
			int i=keylen+1;
			symbol_begin_pos=(str[keylen]==' '?i:i-1);
			for(int j=symbol_begin_pos; str[j]=='*'; j++)
				ptr_level++;
			symbol_begin_pos += ptr_level;
			for(i=symbol_begin_pos; str[i]!='('; i++);
			memcpy(function_name, str + symbol_begin_pos, i - symbol_begin_pos);
			function_name[i - symbol_begin_pos] = 0;

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
					return ERROR_FUNC_ARG_LIST;					
				}
				keylen = strlen(type_key[type]);
				arg_name_begin_pos = i + pos + keylen + (str[i + pos + keylen] == ' ' ? 1 : 0);
				for(int j=arg_name_begin_pos; str[j]=='*'; j++)
					ptr_level++;
				if(type == VOID && !ptr_level) {
					if(arg_stack->get_count() > 1) {
						error("arg list error.\n");
						return ERROR_FUNC_ARG_LIST;
					}
					void_flag = true;
				} else {
					if(void_flag) {
						error("arg cannot use void type.\n");
						return ERROR_FUNC_ARG_LIST;
					}
				}
				arg_name_begin_pos += ptr_level;
				for(int j=arg_name_begin_pos; j<=r_bracket_pos; j++)
					if(str[j] == ',' || str[j] == ')') {
						arg_name_end_pos = j - 1;
						i = j;
						break;
					}
				memcpy(varity_name, str + arg_name_begin_pos, arg_name_end_pos - arg_name_begin_pos + 1);
				varity_name[arg_name_end_pos - arg_name_begin_pos + 1] = 0;
				if(!void_flag) {
					arg_node_ptr->arg_init(varity_name, type, sizeof_type[type], (void*)make_align(offset, sizeof_type[type]));
					arg_stack->push(arg_node_ptr++);
					this->varity_declare->declare(VARITY_SCOPE_LOCAL, varity_name, type, sizeof_type[type], 0);
				} else {
					if(varity_name[0] != 0) {
						error("arg cannot use void type.\n");
						return ERROR_FUNC_ARG_LIST;
					}
				}
				offset = make_align(offset, sizeof_type[type]) + sizeof_type[type];
			}
			//TODO: 释放申请的多余空间
			this->function_declare->declare(function_name, arg_stack);
			this->cur_mid_code_stack_ptr = &this->function_declare->get_current_node()->mid_code_stack;
			this->exec_flag = false;
			this->function_declare->save_sentence(str, len);
			return OK_FUNC_DEFINE;
		}
	}
	return OK_FUNC_NOFUNC;
}

#define RETURN(ret, is_dedeep) arg_string[0] = '('; \
	varity_global_flag = varity_global_flag_backup; \
	this->varity_declare->destroy_local_varity_cur_depth(); \
	if(is_dedeep) \
		this->varity_declare->local_varity_stack->dedeep(); \
	this->analysis_buf_ptr -= this->analysis_buf_inc_stack[--this->call_func_info.function_depth]; \
	this->varity_declare->local_varity_stack->visible_depth = visible_depth_backup; \
	vfree(this->nonseq_info); \
	this->nonseq_info = nonseq_info_backup; \
	return ret
int c_interpreter::call_func(char* name, char* arg_string, uint arg_len)
{
	int ret, arg_count;
	int varity_global_flag_backup = this->varity_global_flag;
	int visible_depth_backup = this->varity_declare->local_varity_stack->visible_depth;
	nonseq_info_struct* nonseq_info_backup = this->nonseq_info;
	this->analysis_buf_ptr += this->analysis_buf_inc_stack[this->call_func_info.function_depth++];
	this->nonseq_info = (nonseq_info_struct*)vmalloc(sizeof(nonseq_info_struct));
	int arg_end_pos = arg_len - 1;
	varity_info* arg_varity;
	this->varity_global_flag = VARITY_SCOPE_LOCAL;
	arg_string[0] = 0;
	function_info* called_function_ptr = this->function_declare->find(name);
	arg_string[0] = ',';
	arg_count = called_function_ptr->arg_list->get_count();
	arg_varity = (varity_info*)vmalloc(arg_count * sizeof(varity_info));//TODO:核查内存泄漏
	//while(arg_count-- > 1) {
	for(int i=arg_count-1; i>=1; i--) {
		varity_attribute* arg_ptr = (varity_attribute*)called_function_ptr->arg_list->visit_element_by_index(i);
		//this->varity_declare->declare(VARITY_SCOPE_LOCAL, arg_ptr->get_name(), arg_ptr->get_type(), arg_ptr->get_size());
		int comma_pos = str_find(arg_string, arg_end_pos, ',', 1);
		if(comma_pos >= 0) {
			this->sentence_exec(arg_string + comma_pos + 1, arg_end_pos - comma_pos - 1, false, &arg_varity[i]);
			//*(varity_info*)this->varity_declare->local_varity_stack->get_lastest_element() = arg_varity;
			//arg_varity[arg_count].reset();
		} else {
			error("Args insufficient\n");
			RETURN(ERROR_FUNC_ARGS, 0);
		}
		arg_end_pos = comma_pos;
	}
	this->varity_declare->local_varity_stack->endeep();//先用完旧局部变量算参数才能endeep
	this->varity_declare->local_varity_stack->visible_depth = this->varity_declare->local_varity_stack->current_depth;
	for(int i=arg_count-1; i>=0; i--) {
		varity_attribute* arg_ptr = (varity_attribute*)called_function_ptr->arg_list->visit_element_by_index(i);
		this->varity_declare->declare(VARITY_SCOPE_LOCAL, arg_ptr->get_name(), arg_ptr->get_type(), arg_ptr->get_size());
		*(varity_info*)this->varity_declare->local_varity_stack->get_lastest_element() = arg_varity[i];
		arg_varity[i].reset();
	}
	vfree(arg_varity);
	varity_attribute* arg_ptr = (varity_attribute*)called_function_ptr->arg_list->visit_element_by_index(0);
	this->function_return_value->init(arg_ptr, "", arg_ptr->get_type(), 0, arg_ptr->get_size());//TODO: check attribute.
	for(int row_line=2; row_line<called_function_ptr->row_line-1; row_line++) {
		ret = sentence_analysis(called_function_ptr->row_begin_pos[row_line], called_function_ptr->row_len[row_line]);
		if(ret < 0) {
			RETURN(ret, 1);
		}
	}
	RETURN(ERROR_NO, 1);
}
#undef RETURN(x)

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
			return true;
		}
		break;
	case OPT_L_MID_BRACKET:
		type = OPT_INDEX;
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

int c_interpreter::generate_mid_code(char *str, uint len, bool need_semicolon)
{
	if(len == 0 || str[0] == '{' || str[0] == '}')return ERROR_NO;
	sentence_analysis_data_struct_t *analysis_data_struct_ptr = &this->sentence_analysis_data_struct;
	int token_len;
	int node_index = 0;
	node_attribute_t *node_attribute, *stack_top_node_ptr;
	this->sentence_analysis_data_struct.last_token.node_type = TOKEN_OPERATOR;
	this->sentence_analysis_data_struct.last_token.value.int_value = OPT_EDGE;
	if(!strmcmp(str, "return ", 7)) {
		int ret;
		mid_code *mid_code_ptr;
		if(this->cur_mid_code_stack_ptr == &this->mid_code_stack)
			return OK_FUNC_RETURN;
		ret = this->generate_mid_code(str + 7, len - 7, true);//下一句貌似重复了，所以注释掉。
		//this->generate_expression_value(this->cur_mid_code_stack_ptr, (node_attribute_t*)this->sentence_analysis_data_struct.tree_root->value);
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		mid_code_ptr->ret_operator = CTL_RETURN;
		this->cur_mid_code_stack_ptr->push();
		return ret;
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
							if(!analysis_data_struct_ptr->expression_final_stack.get_count() || final_stack_top_ptr->node_type == TOKEN_OPERATOR) {
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
						}
						analysis_data_struct_ptr->expression_tmp_stack.push(&analysis_data_struct_ptr->node_struct[node_index]);
						break;
					}
				} else {//Notion: priority of right parenthese should be lowwest.
					analysis_data_struct_ptr->expression_final_stack.push(analysis_data_struct_ptr->expression_tmp_stack.pop());
				}
			}
		} else if(node_attribute->node_type == TOKEN_NAME || node_attribute->node_type == TOKEN_CONST_VALUE) {
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
	node *root = analysis_data_struct_ptr->expression_final_stack.pop();
	if(!root) {
		error("No token found.\n");
		return 0;//TODO:找个合适的返回值
	}
	this->sentence_analysis_data_struct.tree_root = root;
	if(analysis_data_struct_ptr->expression_final_stack.get_count() == 0) {
		generate_expression_value(this->cur_mid_code_stack_ptr, (node_attribute_t*)root->value);
		return ERROR_NO;
	}
	root->link_reset();
	list_stack_to_tree(root, &analysis_data_struct_ptr->expression_final_stack);//二叉树完成
	int ret = this->tree_to_code(root, this->cur_mid_code_stack_ptr);//构造中间代码
	if(ret)return ret;
	if(this->mid_varity_stack.get_count())
		this->mid_varity_stack.pop();
	//root->middle_visit();
	debug("generate code.\n");
	//while(analysis_data_struct_ptr->expression_final_stack.get_count()) {
	//	node_attribute_t *tmp = (node_attribute_t*)analysis_data_struct_ptr->expression_final_stack.pop()->value;
	//	printf("%d ",tmp->node_type);
	//	if(tmp->node_type == TOKEN_NAME)
	//		printf("%s\n",tmp->value.ptr_value);
	//	else
	//		printf("%d %d\n",tmp->value.int_value,opt_number[tmp->value.int_value]);
	//}
	return ERROR_NO;
}

int c_interpreter::generate_expression_value(stack *code_stack_ptr, node_attribute_t *node_attribute)
{
		mid_code *instruction_ptr = (mid_code*)code_stack_ptr->get_current_ptr();
		instruction_ptr->opda_addr = 0;
		instruction_ptr->opda_operand_type = OPERAND_T_VARITY;
		instruction_ptr->ret_operator = OPT_ASSIGN;
		if(node_attribute->node_type == TOKEN_CONST_VALUE) {
			instruction_ptr->opdb_operand_type = OPERAND_CONST;
			instruction_ptr->opdb_varity_type = node_attribute->value_type;
			memcpy(&instruction_ptr->opdb_addr, &node_attribute->value, 8);
		} else if(node_attribute->node_type == TOKEN_NAME) {
			if(node_attribute->value.ptr_value[0] == TMP_VAIRTY_PREFIX) {
				instruction_ptr->opdb_operand_type = OPERAND_T_VARITY;
				instruction_ptr->opdb_addr = 8 * node_attribute->value.ptr_value[1];
				instruction_ptr->opdb_varity_type = ((varity_info*)this->mid_varity_stack.visit_element_by_index(node_attribute->value.ptr_value[1]))->get_type();
			} else {
					varity_info *varity_ptr;
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
		instruction_ptr->opda_varity_type = instruction_ptr->opdb_varity_type;
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

int c_interpreter::sentence_analysis(char* str, uint len)
{
	int ret1, ret2;
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
		if(nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth) {
			switch(nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_info) {
			case 0:
				this->nonseq_start_gen_mid_code(str, len, nonseq_info->row_info_node[nonseq_info->row_num - 1].nonseq_type);
				break;
			case 1:
				this->nonseq_end_gen_mid_code(str, len);
				break;
			case 2:
				this->nonseq_mid_gen_mid_code(str, len);
				break;
			default:
				break;
			}
		} else {
			this->generate_mid_code(str, len, true);
		}
		if(this->cur_mid_code_stack_ptr == &this->mid_code_stack && ret2 == OK_NONSEQ_FINISH) {
			this->varity_global_flag = VARITY_SCOPE_GLOBAL;
			this->nonseq_info->stack_frame_size = this->varity_declare->local_varity_stack->offset;
			this->varity_declare->destroy_local_varity();
		}
	} else if(ret2 == ERROR_NONSEQ_GRAMMER)
		return ret2;
	
	if(nonseq_info->non_seq_exec) {
		debug("exec non seq struct\n");
		nonseq_info->non_seq_exec = 0;
		if(this->exec_flag) {
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
	int i;
	int cur_depth = nonseq_info->row_info_node[nonseq_info->row_num - 1].non_seq_depth;
	if(len == 0)return ERROR_NO;
	mid_code *mid_code_ptr;
	node_attribute_t cur_node;
	get_token(str, &cur_node);
	if(cur_node.node_type != TOKEN_KEYWORD_NONSEQ && str[0] != '}') {
		int ret = this->generate_mid_code(str, len, true);
		if(ret) return ret;
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
				this->generate_mid_code(nonseq_info->row_info_node[i].row_ptr + nonseq_info->row_info_node[i].post_info_c + 1, len, false);
				mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
				mid_code_ptr->ret_operator = CTL_BRANCH;
				mid_code_ptr->opda_addr = ((mid_code*)cur_mid_code_stack_ptr->get_base_addr() + nonseq_info->row_info_node[i].post_info_b) - mid_code_ptr;
				this->cur_mid_code_stack_ptr->push();
				mid_code_ptr = (mid_code*)cur_mid_code_stack_ptr->get_base_addr() + nonseq_info->row_info_node[i].post_info_b + nonseq_info->row_info_node[i].post_info_a;
				mid_code_ptr->opda_addr = (mid_code*)cur_mid_code_stack_ptr->get_current_ptr() - mid_code_ptr;
				nonseq_info->row_info_node[i].finish_flag = 1;
				break;
			}
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
	if(token_node.node_type != TOKEN_OPERATOR || token_node.value.int_value != OPT_L_SMALL_BRACKET) {
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
		nonseq_info->row_info_node[nonseq_info->row_num - 1].post_info_b = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr();
		this->generate_mid_code(str + key_len + first_flag_pos + 1, second_flag_pos, false);
		mid_code_ptr = (mid_code*)this->cur_mid_code_stack_ptr->get_current_ptr();
		nonseq_info->row_info_node[nonseq_info->row_num - 1].post_info_a = mid_code_ptr - (mid_code*)this->cur_mid_code_stack_ptr->get_base_addr() - nonseq_info->row_info_node[nonseq_info->row_num - 1].post_info_b;
		mid_code_ptr->ret_operator = CTL_BRANCH_FALSE;
		this->cur_mid_code_stack_ptr->push();
		break;
	case NONSEQ_KEY_WHILE:
		break;
	case NONSEQ_KEY_DO:
		break;
	}
	return ERROR_NO;
}

int c_interpreter::nesting_nonseq_section_exec(int line_begin, int line_end)
{
	int single_sentence_ret;
	int row_line;
	for(row_line=line_begin+1; row_line<=line_end; row_line++) {
		if(nonseq_info->row_info_node[row_line].non_seq_depth > nonseq_info->row_info_node[line_begin].non_seq_depth && nonseq_info->row_info_node[row_line].non_seq_info == 0) {
			int i;
			for(i=row_line+1; i<=line_end; i++) {
				if(nonseq_info->row_info_node[i].non_seq_depth < nonseq_info->row_info_node[row_line].non_seq_depth && nonseq_info->row_info_node[i].non_seq_depth || nonseq_info->row_info_node[i].non_seq_depth == nonseq_info->row_info_node[row_line].non_seq_depth && nonseq_info->row_info_node[i].non_seq_info == 1) {
					non_seq_section_exec(row_line, i);
					break;
				}
			}
			row_line = i;
			continue;
		}
		single_sentence_ret = this->sentence_exec(nonseq_info->row_info_node[row_line].row_ptr, nonseq_info->row_info_node[row_line].row_len, 1, 0);
		if(single_sentence_ret < 0)
			return single_sentence_ret;
	}
	return 0;
}

#define RETURN(x)	this->varity_global_flag = varity_global_flag_backup; \
	this->varity_declare->destroy_local_varity_cur_depth(); \
	this->varity_declare->local_varity_stack->dedeep(); \
	varity_info::en_echo = 1; \
	return x
int c_interpreter::non_seq_section_exec(int line_begin, int line_end)
{
	int varity_global_flag_backup = this->varity_global_flag;
	int row_line;
	int section_type;
	char* row_ptr;
	varity_info condition_varity;
	this->varity_global_flag = VARITY_SCOPE_LOCAL;
	this->varity_declare->local_varity_stack->endeep();
	row_line = line_begin;
	row_ptr = nonseq_info->row_info_node[row_line].row_ptr;
	section_type = non_seq_struct_check(row_ptr);
	varity_info::en_echo = 0;
	if(section_type == NONSEQ_KEY_FOR) {
		int l_bracket_pos = str_find(row_ptr, '(');
		int semi_pos_1st = str_find(row_ptr + l_bracket_pos + 1, ';');
		int semi_pos_2nd = str_find(row_ptr + l_bracket_pos + semi_pos_1st + 2, ';');
		int r_bracket_pos = str_find(row_ptr + l_bracket_pos + semi_pos_1st + semi_pos_2nd + 3, ')');
		int block_ret;
		this->sentence_exec(row_ptr + l_bracket_pos + 1, semi_pos_1st + 1, true, 0);
		while(1) {
			this->sentence_exec(row_ptr + l_bracket_pos + semi_pos_1st + 2, semi_pos_2nd, false, &condition_varity);
			int condition_type = condition_varity.get_type();
			if(condition_type == DOUBLE || condition_type == FLOAT){
				error("Float cannot become condtion\n");
				RETURN(ERROR_CONDITION_TYPE);
			}
			if(!condition_varity.is_non_zero())
				break;
			block_ret = nesting_nonseq_section_exec(line_begin, line_end);
			if(block_ret < 0) {
				error("for loop exec error!\n");
				RETURN(block_ret);
			}
			this->sentence_exec(row_ptr + l_bracket_pos + semi_pos_1st + semi_pos_2nd + 3, r_bracket_pos, false, 0);
		}
	} else if(section_type == NONSEQ_KEY_IF) {
		int l_bracket_pos = str_find(row_ptr, '(');
		int r_bracket_pos = str_find(row_ptr, ')', 1);
		int else_line;
		int condition_type;
		int block_ret;
		for(else_line=line_begin+1; else_line<line_end; else_line++)
			if(nonseq_info->row_info_node[else_line].non_seq_info == 2 && nonseq_info->row_info_node[else_line].non_seq_depth == nonseq_info->row_info_node[line_begin].non_seq_depth)
				break;
		this->sentence_exec(row_ptr + l_bracket_pos + 1, r_bracket_pos - l_bracket_pos - 1, false, &condition_varity);
		condition_type = condition_varity.get_type();
		if(condition_type == DOUBLE || condition_type == FLOAT){
			error("Float cannot become condtion\n");
			RETURN(ERROR_CONDITION_TYPE);
		}
		if(condition_varity.is_non_zero()) {
			debug("condition 1\n");
			if(else_line < line_end)
				block_ret = nesting_nonseq_section_exec(line_begin, else_line - 1);
			else
				block_ret = nesting_nonseq_section_exec(line_begin, line_end);
			if(block_ret < 0) {
				if(block_ret != OK_FUNC_RETURN)
					error("if block exec error!\n");
				RETURN(block_ret);
			}
		} else {
			if(else_line < line_end)
				block_ret = nesting_nonseq_section_exec(else_line, line_end);
			else
				block_ret = 0;
			if(block_ret < 0) {
				if(block_ret != OK_FUNC_RETURN)
					error("if block exec error!\n");
				RETURN(block_ret);
			}
		}
	} else if(section_type == NONSEQ_KEY_WHILE) {
		int l_bracket_pos = str_find(row_ptr, '(');
		int r_bracket_pos = str_find(row_ptr, ')', 1);
		int condition_type;
		int block_ret;
		while(1) {
			this->sentence_exec(row_ptr + l_bracket_pos + 1, r_bracket_pos - l_bracket_pos - 1, false, &condition_varity);
			condition_type = condition_varity.get_type();
			if(condition_type == DOUBLE || condition_type == FLOAT){
				error("Float cannot become condtion\n");
				RETURN(ERROR_CONDITION_TYPE);
			}
			if(!condition_varity.is_non_zero())
				break;
			debug("while condition 1\n");
			block_ret = nesting_nonseq_section_exec(line_begin, line_end);
			if(block_ret < 0) {
				error("while loop exec error!\n");
				RETURN(block_ret);
			}
		}
	}
	//}

	RETURN(0);
}
#undef RETURN(x)

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
				key_len = strlen(type_key[varity_type]);
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
							memcpy(varity_name, str + symbol_pos_last, symbol_pos_once);
							varity_name[symbol_pos_once] = 0;
							array_flag = 1;
						} else if(opt_type == OPT_R_MID_BRACKET) {
							array_flag = 2;
							element_count = y_atoi(str + symbol_pos_last, symbol_pos_once);
						} else {
							if(array_flag == 0) {
								memcpy(varity_name, str + symbol_pos_last, symbol_pos_once);
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

				memcpy(varity_name, str + varity_name_begin_pos, len - varity_name_begin_pos + 1);
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
			int keylen = strlen(type_key[is_varity_declare]);
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
			memcpy(struct_name, str + symbol_begin_pos, len - symbol_begin_pos);
			struct_name[len - symbol_begin_pos] = 0;
			varity_attribute* arg_node_ptr = (varity_attribute*)vmalloc(sizeof(varity_info) * MAX_VARITY_COUNT_IN_STRUCT);
			arg_stack = (stack*)vmalloc(sizeof(stack));
			memcpy(arg_stack, &stack(sizeof(varity_info), arg_node_ptr, MAX_VARITY_COUNT_IN_STRUCT), sizeof(stack));
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
	struct_info* struct_node_ptr;
	if(is_varity_declare >= 0) {
		int key_len = strlen(type_key[is_varity_declare]);
		if(is_varity_declare != STRUCT)
			len = remove_char(str + key_len + 1, ' ') + key_len + 1;
		else {//TODO: 处理结构名后接*的情况
			int space_2nd_pos = str_find(str + key_len + 1, len - key_len - 1, ' ') + key_len + 1;
			if(str[space_2nd_pos - 1] == '*')
				space_2nd_pos--;
			memcpy(struct_name, str + key_len + 1, space_2nd_pos - key_len - 1);
			struct_name[space_2nd_pos - key_len - 1] = 0;
			struct_node_ptr = this->struct_declare->find(struct_name);
			if(!struct_node_ptr) {
				error("There is no struct called %s.\n", struct_name);
				return ERROR_STRUCT_NONEXIST;
			}
			len = remove_char(str + space_2nd_pos + 1, ' ') + space_2nd_pos + 1;
			key_len = space_2nd_pos;
		}
		/////////////////
		char varity_name[32];
		varity_info* new_varity_ptr;
		int array_flag = 0, element_count = 1, ptr_level = 0;
		int opt_len, opt_type, symbol_pos_once, symbol_pos_last = str[key_len]==' '?key_len+1:key_len, size = len - symbol_pos_last;
		while(size > 0) {
			for(; str[symbol_pos_last]=='*'; symbol_pos_last++)
				ptr_level++;
			size -= ptr_level;
			if((symbol_pos_once = search_opt(str + symbol_pos_last, size, 0, &opt_len, &opt_type)) >= 0) {
				if(opt_type == OPT_L_MID_BRACKET) {
					memcpy(varity_name, str + symbol_pos_last, symbol_pos_once);
					varity_name[symbol_pos_once] = 0;
					array_flag = 1;
				} else if(opt_type == OPT_R_MID_BRACKET) {
					array_flag = 2;
					element_count = y_atoi(str + symbol_pos_last, symbol_pos_once);
				} else {
					if(array_flag == 0) {
						memcpy(varity_name, str + symbol_pos_last, symbol_pos_once);
						varity_name[symbol_pos_once] = 0;
					} else if(array_flag == 2) {

					} else {
						
					}
					int ret;
					if(this->varity_global_flag == VARITY_SCOPE_GLOBAL) {
						if(ptr_level)
							ret = this->varity_declare->declare(VARITY_SCOPE_GLOBAL, varity_name, is_varity_declare + ptr_level * BASIC_VARITY_TYPE_COUNT, PLATFORM_WORD_LEN * element_count);
						else
							ret = this->varity_declare->declare(VARITY_SCOPE_GLOBAL, varity_name, is_varity_declare, sizeof_type[is_varity_declare] * element_count);
						new_varity_ptr = (varity_info*)this->varity_declare->global_varity_stack->get_lastest_element();
					} else {
						if(ptr_level)
							ret = this->varity_declare->declare(VARITY_SCOPE_LOCAL, varity_name, is_varity_declare + ptr_level * BASIC_VARITY_TYPE_COUNT, PLATFORM_WORD_LEN * element_count);
						else
							ret = this->varity_declare->declare(VARITY_SCOPE_LOCAL, varity_name, is_varity_declare, sizeof_type[is_varity_declare] * element_count);
						new_varity_ptr = (varity_info*)this->varity_declare->local_varity_stack->get_lastest_element();
					}
					if(ret)
						return ret;
					if(is_varity_declare == STRUCT) {
						new_varity_ptr->config_varity(0, struct_node_ptr);
						new_varity_ptr->struct_apply();
					}

					array_flag = 0;
					element_count = 1;
					ptr_level = 0;
					if(opt_type == OPT_ASSIGN) {
						int comma_pos;
						symbol_pos_last += symbol_pos_once + opt_len;
						for(comma_pos=symbol_pos_last; comma_pos<len; comma_pos++) {
							if(str[comma_pos] == ',' || str[comma_pos] == ';') {
								break;
							}
						}
						char assign_exec_buf[VARITY_ASSIGN_BUFLEN];
						char* analysis_buf_ptr_backup = this->analysis_buf_ptr;
						this->analysis_buf_ptr = assign_exec_buf;
						this->sentence_exec(str + symbol_pos_last, comma_pos - symbol_pos_last, false, new_varity_ptr);
						this->analysis_buf_ptr = analysis_buf_ptr_backup;
						size -= symbol_pos_once + opt_len + comma_pos - symbol_pos_last + 1;
						symbol_pos_last = comma_pos + 1;
						new_varity_ptr->echo();
						continue;
					}
				}
			} else {
				break;
			}
			symbol_pos_last += symbol_pos_once + opt_len;
			size -= symbol_pos_once + opt_len;
		}
		return OK_VARITY_DECLARE;
	}
	return ERROR_NO;
}

int c_interpreter::sentence_exec(char* str, uint len, bool need_semicolon, varity_info* expression_value)
{
	int i, ret;
	int total_bracket_depth;
	uint analysis_varity_count;
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
	total_bracket_depth = get_bracket_depth(analysis_buf_ptr);
	if(total_bracket_depth < 0)
		return ERROR_BRACKET_UNMATCH;
	strcpy(this->analysis_buf_ptr, str);
	analysis_varity_count = this->varity_declare->analysis_varity_stack->get_count();
	int key_word_ret = key_word_analysis(this->analysis_buf_ptr, len);
	if(key_word_ret) {
		str[source_len] = ch_last;
		return key_word_ret;
	}
	ret = this->generate_mid_code(str, len, true);
	if(this->exec_flag) {
		int mid_code_count = this->cur_mid_code_stack_ptr->get_count();
		this->exec_mid_code((mid_code*)this->cur_mid_code_stack_ptr->get_base_addr(), mid_code_count);
		this->cur_mid_code_stack_ptr->empty();
	}
	//for(int n=0; n<mid_code_count; n++) {
	//	((mid_code*)this->cur_mid_code_stack_ptr->visit_element_by_index(n))->exec_code(this->stack_pointer, this->tmp_varity_stack_pointer);
	//}
	return 0;
	uint current_analysis_varity_count = this->varity_declare->analysis_varity_stack->get_count();
	this->varity_declare->destroy_analysis_varity(current_analysis_varity_count - analysis_varity_count);
	str[source_len] = ch_last;
	return ERROR_NO;
}

int c_interpreter::sub_sentence_analysis(char* str, uint *len)//无括号或仅含类型转换的子句解析
{
	int i;
	for(i=0; i<C_OPT_PRIO_COUNT; i++)
		if(this->c_opt_caculate_func_list[i]) {
			int ret = (this->*c_opt_caculate_func_list[i])(str,len);
			if(ret < 0)
				return ret;
			//debug("opt_prio=%d\n", i);
		}
	return ERROR_NO;
}

int c_interpreter::non_seq_struct_check(char* str)
{
	return nonseq_key_cmp(str);
}
