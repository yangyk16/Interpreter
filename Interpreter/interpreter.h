#pragma once
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "type.h"
#include "hal.h"

class interpreter {
protected:
	char sentence_buf[MAX_SENTENCE_LENGTH];
	terminal* tty_used;
	bool print_ret;

	void remove_extra_char(void);
	int call_func(char*, uint);
	virtual int sentence_analysis(char*) = 0;
public:
	int run_interpreter(void);
};

class c_interpreter: public interpreter {
	virtual int sentence_analysis(char*);
public:
	c_interpreter(terminal*);
};

#endif
