#pragma once
#ifndef GDB_H
#define GDB_H

class c_interpreter;

#define MAX_GDBCMD_ARGC 8
#define ARG_SPACE_SIZE	256

class gdb {
	static int argc;
	static char *argv[MAX_GDBCMD_ARGC];
	static char args[ARG_SPACE_SIZE - sizeof(argv)];
public:
	static int parse(char *cmd_str);
	static int exec(c_interpreter* interpreter_ptr);

	static int print_code(int argc, char **argv, c_interpreter *cptr);
};

#endif