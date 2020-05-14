#pragma once
#ifndef TEXT_H
#define TEXT_H

#include "config.h"

#define INDENT_REASON_NONSEQ	1
#define INDENT_REASON_BRACKET	2
#define INDENT_REASON_NOTCARE	3

class indentator {
	char indentation;
	char reason[MAX_INDENTATION];
	//short rows;
public:
	int auto_indent(char *str, unsigned int len = 0);
	int auto_indent(char *str, short &cur_indentation, unsigned int &len);
	void change_indent(char increase_level, char reason);
};
#endif