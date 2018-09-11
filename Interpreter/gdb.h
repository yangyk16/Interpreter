#pragma once
#ifndef GDB_H
#define GDB_H

class c_interpreter;
class mid_code;

#define MAX_GDBCMD_ARGC 8
#define ARG_SPACE_SIZE	256
#define MAX_ARG_LEN		8

#define FORMAT_AUTO		0
#define FORMAT_HEX		1
#define FORMAT_DEC		2
#define FORMAT_UINT		3
#define FORMAT_FLOAT	4
#define FORMAT_CHAR		5

class gdb {
	static int trace_en;
	static int argc;
	static char *argv[MAX_GDBCMD_ARGC];
	static char args[ARG_SPACE_SIZE - sizeof(argv)];
	static char argstr[MAX_ARG_LEN];
	static char *wptr;
	static mid_code *bp_todo;
	static void *get_real_addr(void *addr, int type, c_interpreter *cptr);
public:
	static int get_trace_status(void) { return gdb::trace_en; }
	static int parse(char *cmd_str);
	static int breakpoint_handle(c_interpreter *interpreter_ptr, mid_code *mid_code_ptr);
	static int exec(c_interpreter* interpreter_ptr);
	static void clear_bp_todo(void) {bp_todo = 0;}
	static mid_code *get_bp_todo(void) {return bp_todo;}

	static int trace_ctl(int argc, char **argv, c_interpreter *cptr);
	static int print_code(int argc, char **argv, c_interpreter *cptr);
	static int print_mid_code(int argc, char **argv, c_interpreter *cptr);
	static int breakpoint(int argc, char **argv, c_interpreter *cptr);
	static int print(int argc, char **argv, c_interpreter *cptr);
};

int breakpoint_handle(c_interpreter *interpreter_ptr, mid_code *mid_code_ptr);
#endif