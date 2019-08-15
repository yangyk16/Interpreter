#include "interpreter.h"
#include "data_struct.h"

#if TTY_TYPE == 0
tty stdio;
#elif TTY_TYPE == 1
uart stdio;
#endif
file fileio;
stack string_stack;
stack name_stack;
strfifo name_fifo;
c_interpreter myinterpreter;
c_interpreter irq_interpreter;
varity_type_stack_t varity_type_stack = {0};