#include "text.h"
#include "cstdlib.h"
#include "interpreter.h"

int indentator::auto_indent(char *str, unsigned int len)
{
	short i = 0;
	if(len == 0)
		len = kstrlen(str);
	while(kisspace(*str++))
		i++;
	return this->auto_indent(str, i, len);
}

int indentator::auto_indent(char *str, short &cur_indentation, unsigned int &len)
{
	int ret = 0, indentation;
	node_attribute_t token_node;
	get_token(str, &token_node);
	if(token_node.node_type == TOKEN_OTHER) {
		if(token_node.data == L_BIG_BRACKET) {
			if(this->indentation > 0 && this->reason[this->indentation - 1] == INDENT_REASON_NONSEQ) {
				indentation = this->indentation - 1;
			} else {
				indentation = this->indentation;
			}
		} else if(token_node.data == R_BIG_BRACKET) {
			indentation = this->indentation - 1;
		} else {
			indentation = this->indentation;
		}
	} else {
		indentation = this->indentation;
	}
	if(cur_indentation != indentation) {
		kmemmove(str + indentation, str + cur_indentation, len - cur_indentation + 1);
		ret = indentation - cur_indentation;
		len += ret;
		cur_indentation = indentation;
	}
	kmemset(str, '\t', indentation);
	return ret;
}

void indentator::change_indent(char increase_level, char reason)
{
	this->indentation += increase_level;
	if(increase_level == 1)
		this->reason[indentation - 1] = reason;
}
