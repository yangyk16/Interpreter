#include "interpreter.h"
#include "data_struct.h"

#if TTY_TYPE == 0
tty stdio(0);
#elif TTY_TYPE == 1
uart stdio(1);
#endif
file fileio(0);
file lfileio(0);
stack string_stack;
stack name_stack;
strfifo name_fifo;
strfifo string_fifo;
c_interpreter myinterpreter;
c_interpreter irq_interpreter;
varity_type_stack_t varity_type_stack = {0};
