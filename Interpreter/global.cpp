#include "interpreter.h"
#include "data_struct.h"
#include "global.h"

stack string_stack;
stack name_stack;
strfifo name_fifo;
c_interpreter myinterpreter;
c_interpreter irq_interpreter;