#pragma once
#ifndef GLOBAL_H
#define GLOBAL_H

#include "config.h"
#include "interpreter.h"

#if TTY_TYPE == 0
extern tty stdio;
#elif TTY_TYPE == 1
extern uart stdio;
#endif
extern file fileio;
extern stack string_stack;
extern stack name_stack;
extern strfifo name_fifo;
extern c_interpreter myinterpreter;
extern c_interpreter irq_interpreter;
extern varity_type_stack_t varity_type_stack;
#endif