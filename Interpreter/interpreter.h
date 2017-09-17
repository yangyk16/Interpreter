#pragma once
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "type.h"
#include "hal.h"
#include "config.h"
#include "varity.h"

typedef varity_info (*opt_calc)(char*,uint);
class interpreter {
protected:
	varity* varity_declare;
	varity* temp_varity_analysis;
	char sentence_buf[MAX_SENTENCE_LENGTH];
	char analysis_buf[MAX_ANALYSIS_BUFLEN];
	terminal* tty_used;
	bool print_ret;

	virtual void remove_extra_char(void){};
	int call_func(char*, uint);
	virtual int sentence_analysis(char*) = 0;
public:
	int run_interpreter(void);
};

class c_interpreter: public interpreter {
	opt_calc *opt_caculate_func_list;
	int sub_sentence_analysis(char*, uint size);
	virtual int sentence_analysis(char*);
	virtual void remove_extra_char(void);
public:
	c_interpreter(terminal*);
};

#endif
