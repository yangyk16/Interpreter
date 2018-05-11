#include "gdb.h"
#include "error.h"
#include "config.h"
#include "cstdlib.h"
#include "interpreter.h"

#if DEBUG_EN
typedef int (*cmd_fun)(int argc, char** argv, c_interpreter*);
typedef struct cmd {
	const char* str;
	cmd_fun fun;
} cmd_t;

int gdb::argc;
char *gdb::argv[MAX_GDBCMD_ARGC];
char gdb::args[ARG_SPACE_SIZE - sizeof(argv)];

inline int IsSpace(char ch) {return (ch == ' ' || ch == '\t' || ch == '/');}

static int print(int argc, char **argv, c_interpreter *cptr)
{

	return ERROR_NO;
}

static int continue_exec(int argc, char **argv, c_interpreter *cptr)
{

	return ERROR_NO;
}

static int breakpoint(int argc, char **argv, c_interpreter *cptr)
{
	debug("set breakpoint.\n");
	return ERROR_NO;
}

int gdb::print_code(int argc, char **argv, c_interpreter *cptr)
{
	function_info *fptr;
	fptr = cptr->function_declare->find(argv[1]);
	if(!fptr)
		gdbout("Function not found.\n");
	else {
		for(int i=0; i<fptr->row_line; i++)
			fptr->print_line(i);
	}
	return ERROR_NO;
}

cmd_t cmd_tab[] = {
	{"print", print},
	{"p", print},
	{"b", breakpoint},
	{"c", continue_exec},
	{"pcode", gdb::print_code},
};

int gdb::parse(char *cmd_str)
{
begin:
	int char_index = 0;
	int char_total_index = 0;
	char last_char = ' ';
	bool inword = false;
	argc = -1;
	char ch;
	while(*cmd_str)
	{
		ch = *cmd_str++;
		if(!IsSpace(ch) && IsSpace(last_char)) {
			if(argc >= 0) {
				char_total_index++;
				if(char_total_index > ARG_SPACE_SIZE) {
					debug("Cmd error: cmd too long\r\n");
					return -3;
				}
				argv[argc][char_index] = 0;
			}
			char_index = 0;
			argc++;
			if(argc >= MAX_GDBCMD_ARGC) {
				debug("Cmd error: too many args\r\n");
				return -2;
			}
			argv[argc] = &args[char_total_index];
			inword = true;
		}
		if(IsSpace(ch) && !IsSpace(last_char)) {
			inword = false;
		}
		if(inword) {
			char_total_index++;
			if(char_total_index > ARG_SPACE_SIZE) {
				debug("Cmd error: cmd too long\r\n");
				return -3;
			}			
			argv[argc][char_index++] = ch;
		}
		last_char = ch;
	}
	if(argc >= 0)
		argv[argc][char_index] = 0;
	else
		goto begin;
	argc++;
	return argc;
}

int gdb::exec(c_interpreter* interpreter_ptr)
{
	static int cmd_count = sizeof(cmd_tab) / sizeof(cmd_t);
	int i;
	if(argc <= 0)return -1;
	for(i=0; i<cmd_count; i++)
		if(!kstrcmp(cmd_tab[i].str, argv[0]) && cmd_tab[i].fun != NULL)
			return cmd_tab[i].fun(argc, argv, interpreter_ptr);
	debug("Wrong Cmd: %s\r\n", argv[0]);
	return -1;
}
#endif
