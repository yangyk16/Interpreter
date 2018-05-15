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

typedef struct bp_info {
	static int total_no;
	mid_code *bp_addr;
	int no;
	char en_flag;
} bp_info_t;

int gdb::argc;
char *gdb::argv[MAX_GDBCMD_ARGC];
char gdb::args[ARG_SPACE_SIZE - sizeof(argv)];

int bp_info_t::total_no = 0;
list_stack bp_stack;

inline int IsSpace(char ch) {return (ch == ' ' || ch == '\t' || ch == '/');}

static int print(int argc, char **argv, c_interpreter *cptr)
{

	return ERROR_NO;
}

static int continue_exec(int argc, char **argv, c_interpreter *cptr)
{
	return OK_GDB_RUN;
}

static int step_code(int argc, char **argv, c_interpreter *cptr)
{
	return OK_GDB_STEPRUN_CODE;
}

static int stepover(int argc, char **argv, c_interpreter *cptr)
{
	return OK_GDB_STEPOVER;
}

int gdb::breakpoint(int argc, char **argv, c_interpreter *cptr)
{
	function_info *fptr;
	int line;
	if(argc != 3)
		return ERROR_GDB_ARGC;
	line = katoi(argv[2]);
	//if(!kstrcmp(argv[1], "main")) {
	//	for(int i=0; i<cptr->nonseq_info->row_num; i++)
	//		kprintf("%03d %s\n", i, cptr->nonseq_info->row_info_node[i].row_ptr);
	//	return ERROR_NO;
	//}
	fptr = cptr->function_declare->find(argv[1]);
	if(!fptr)
		gdbout("Function not found.\n");
	else {
		if(fptr->row_code_ptr[line]->break_flag & BREAKPOINT_REAL) {
			gdbout("Breakpoint duplicated.\n");
			return ERROR_NO;
		}
		fptr->row_code_ptr[line]->break_flag |= BREAKPOINT_REAL;
		bp_info_t *bp_info_ptr = (bp_info_t*)vmalloc(sizeof(bp_info_t));
		node *bpnode_ptr = (node*)vmalloc(sizeof(node));
		bpnode_ptr->value = bp_info_ptr;
		bp_info_ptr->no = ++bp_info_ptr->total_no;
		bp_info_ptr->bp_addr = fptr->row_code_ptr[line];
		bp_stack.push(bpnode_ptr);
	}
	return ERROR_NO;
}

int del_breakpoint(int argc, char **argv, c_interpreter *cptr)
{
	if(argc != 2)
		return ERROR_GDB_ARGC;
	int no = katoi(argv[1]);
	node *nptr;
	int i;
	for(nptr=bp_stack.get_head()->right, i=0; i<=bp_stack.get_count(); i++, nptr=nptr->right) {
		if(((bp_info_t*)nptr->value)->no == no) {
			bp_stack.del(nptr);
			return ERROR_NO;
		}
	}
	return ERROR_NO;
}

int info_ask(int argc, char **argv, c_interpreter *cptr)
{
	if(!kstrcmp(argv[1], "break")) {
		int i;
		node *nptr;
		for(nptr=bp_stack.get_head()->right, i=0; i<bp_stack.get_count(); i++, nptr=nptr->right) {
			gdbout("breakpoint @ 0x%x, id = %d\n", ((bp_info_t*)nptr->value)->bp_addr, ((bp_info_t*)nptr->value)->no);
		}
	}
	return ERROR_NO;
}

int gdb::print_code(int argc, char **argv, c_interpreter *cptr)
{
	function_info *fptr;
	if(argc == 1)
		return ERROR_NO;
	if(!kstrcmp(argv[1], "main")) {
		for(int i=0; i<cptr->nonseq_info->row_num; i++)
			gdbout("%03d %s\n", i, cptr->nonseq_info->row_info_node[i].row_ptr);
		return ERROR_NO;
	}
	fptr = cptr->function_declare->find(argv[1]);
	if(!fptr)
		gdbout("Function not found.\n");
	else {
		for(int i=0; i<fptr->row_line; i++)
			fptr->print_line(i);
	}
	return ERROR_NO;
}

int gdb::print_mid_code(int argc, char **argv, c_interpreter *cptr)
{
	function_info *fptr;
	if(argc == 1)
		return ERROR_NO;
	if(!kstrcmp(argv[1], "main")) {
		cptr->print_code((mid_code*)cptr->mid_code_stack.get_base_addr(), cptr->mid_code_stack.get_count());
		return ERROR_NO;
	}
	fptr = cptr->function_declare->find(argv[1]);
	if(!fptr)
		gdbout("Function not found.\n");
	else {
		cptr->print_code((mid_code*)fptr->mid_code_stack.get_base_addr(), fptr->mid_code_stack.get_count());
	}
	return ERROR_NO;
}

cmd_t cmd_tab[] = {
	{"print", print},
	{"p", print},
	{"b", gdb::breakpoint},
	{"c", continue_exec},
	{"d", del_breakpoint},
	{"n", stepover},
	{"ni", step_code},
	{"si", step_code},
	{"info", info_ask},
	{"pcode", gdb::print_code},
	{"pmcode", gdb::print_mid_code},
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
					gdbout("Cmd error: cmd too long\r\n");
					return -3;
				}
				argv[argc][char_index] = 0;
			}
			char_index = 0;
			argc++;
			if(argc >= MAX_GDBCMD_ARGC) {
				gdbout("Cmd error: too many args\r\n");
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
				gdbout("Cmd error: cmd too long\r\n");
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
	gdbout("Wrong Cmd: %s\r\n", argv[0]);
	return -1;
}

int gdb::breakpoint_handle(c_interpreter *interpreter_ptr, mid_code *instruction_ptr)
{
	int gdbret;
	if(instruction_ptr->break_flag) {
		char gdbstr[128];
		int line, tret;
		function_info *fptr;
		if(instruction_ptr->break_flag & BREAKPOINT_REAL)
			gdbout("Breakpoint @ 0x%x.\n", instruction_ptr);
		tret = interpreter_ptr->find_fptr_by_code(instruction_ptr, fptr, &line);
		if(tret) {
			error("text section broken.\n");
			return ERROR_ILLEGAL_CODE;
		}
		gdbout("Running to function %s line %d.\n", (fptr ? fptr->get_name() : "main"), line);
		while(1) {
			instruction_ptr->break_flag &= ~BREAKPOINT_STEP;
			gdbout("gdb>");
			interpreter_ptr->tty_used->readline(gdbstr);
			gdb::parse(gdbstr);
			gdbret = gdb::exec(interpreter_ptr);
			if(gdbret == OK_GDB_RUN || gdbret == OK_GDB_STEPRUN_CODE || gdbret == OK_GDB_STEPINTO || gdbret == OK_GDB_STEPOVER) {
				if(gdbret == OK_GDB_STEPOVER) {
					if(fptr && line != fptr->row_line - 1) {
						fptr->row_code_ptr[line + 1]->break_flag |= BREAKPOINT_STEP;
						break;
					} else {
						((mid_code*)(*((PLATFORM_WORD*)interpreter_ptr->stack_pointer - 1)) + 1)->break_flag |= BREAKPOINT_STEP;
					}
				}
				break;
			}
		}
		return gdbret;
	}
}
#endif
