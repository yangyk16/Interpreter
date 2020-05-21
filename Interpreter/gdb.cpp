#include "gdb.h"
#include "error.h"
#include "config.h"
#include "cstdlib.h"
#include "interpreter.h"
#include "string_lib.h"

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

int gdb::trace_en = 0;
int gdb::argc;
char *gdb::argv[MAX_GDBCMD_ARGC];
char gdb::args[ARG_SPACE_SIZE - sizeof(argv)];
mid_code *gdb::bp_todo = 0;
char *gdb::wptr = 0;
char gdb::argstr[MAX_ARG_LEN];
short gdb::print_cfg = PRINT_CFG_ALL;
short gdb::near_rows = 16;

int bp_info_t::total_no = 0;
list_stack bp_stack;

void *gdb::get_real_addr(void *addr, int type, c_interpreter *cptr)
{
	switch(type) {
	case VARITY_SCOPE_TMP:
		return (void*)(cptr->tmp_varity_stack_pointer - (char*)addr - 8);
		break;
	case VARITY_SCOPE_GLOBAL:
		return addr;
		break;
	case VARITY_SCOPE_LOCAL:
		return cptr->stack_pointer + (PLATFORM_WORD)addr;
		break;
	}
	return addr;
}

int gdb::trace_ctl(int argc, char **argv, c_interpreter *cptr)
{
	if(argc < 2)
		return ERROR_GDB_ARGC;
	gdb::trace_en = katoi(argv[1]);
	return ERROR_NO;
}

int gdb::print(int argc, char **argv, c_interpreter *cptr)
{
	int ret;
	int format = 0;
	varity_info *varity_ptr;
	function_info *function_ptr;
	indexed_stack *lvsp_bak = cptr->varity_declare->local_varity_stack;
	cptr->find_fptr_by_code(cptr->pc, function_ptr, 0);
	cptr->varity_declare->local_varity_stack = &function_ptr->local_varity_stack;
	if(argc < 2)
		return ERROR_GDB_ARGC;
	ret = cptr->open_eval(argv[1], false);//TODO:后续算临时变量值时应当把栈向前推进一帧。
	if(!ret) {
		switch(((node_attribute_t*)cptr->interprete_need_ptr->sentence_analysis_data_struct.tree_root->value)->node_type) {
		case TOKEN_NAME:
		{
			if(argstr[0] == 0) {
				format = 0;
			} else {
				switch(argstr[1]) {
				case 'x':
				case 'a':
					format = FORMAT_HEX;
					break;
				case 'd':
					format = FORMAT_DEC;
					break;
				case 'u':
					format = FORMAT_UINT;
					break;
				case 'f':
					format = FORMAT_FLOAT;
					break;
				case 'c':
					format = FORMAT_CHAR;
					break;
				}
			}
			char *name = ((node_attribute_t*)cptr->interprete_need_ptr->sentence_analysis_data_struct.tree_root->value)->value.ptr_value;
			int scope;
			
			if(name[0] == TMP_VAIRTY_PREFIX || name[0] == LINK_VARITY_PREFIX) {
				varity_ptr = (varity_info*)cptr->interprete_need_ptr->mid_varity_stack.visit_element_by_index(0);
				scope = VARITY_SCOPE_TMP;
			} else {
				varity_ptr = cptr->varity_declare->vfind(name, scope);
			}
			
			if(varity_ptr->get_type() == ARRAY) {
				gdbout("%s = \n", name);
				print_varity(format, varity_ptr->get_complex_arg_count(), varity_ptr->get_complex_ptr(), get_real_addr(varity_ptr->get_content_ptr(), scope, cptr));
			} else {
				gdbout("%s = ", argv[1]);
				print_varity(format, varity_ptr->get_complex_arg_count(), varity_ptr->get_complex_ptr(), get_real_addr(varity_ptr->get_content_ptr(), scope, cptr));
				gdbout("\n");
			}
			break;
		}
		case TOKEN_CONST_VALUE:
			break;
		}
	}
	cptr->varity_declare->local_varity_stack = lvsp_bak;
	return ERROR_NO;
}

static int continue_exec(int argc, char **argv, c_interpreter *cptr)
{
	return OK_GDB_RUN;
}

static int gdbset(int argc, char **argv, c_interpreter *cptr)
{
	char *ptr;
	node_attribute_t node;
	int token_len;
	if(argv[1][0] == '*') {
		token_len = get_token(&argv[1][1], &node);
		if(node.node_type == TOKEN_CONST_VALUE) {

		} else {

		}
	} else if(argv[1][0] == '$') {

	} else {

	}
	return 0;
}

static int print_call_stack(int argc, char **argv, c_interpreter *cptr)
{
	return cptr->print_call_stack();
}

int gdb::memory_dump(int argc, char **argv, c_interpreter *cptr)
{
	int width = 4;
	int count = 1;
	int format = 0;
	unsigned long addr;
	int argvptr, i;
	char *end;
	if(argc < 2)
		error("Too few arguments for \"x\"\n");
	addr = katoi(argv[1]);

	if(gdb::argstr[0]) {
		end = &gdb::argstr[1];
		while(kisdigit(*end))
			end++;
		char bak = *end;
		*end = 0;
		count = katoi(&gdb::argstr[1]);
		*end = bak;
		switch(*end) {
		case 'b':
			width = 1;
			break;
		case 'h':
			width = 2;
			break;
		case 'w':
			width = 4;
			break;
		case 'g':
			width = 8;
			break;
		}
		count *= width;
	}
	for(i=0; i<count; i+=width) {
		if(i % 16 == 0)
			gdbout("0x%08x:", addr + i);
		switch(width) {
		case 1:
			gdbout("%02x ", U_CHAR_VALUE(addr + i));
			break;
		case 2:
			gdbout("%04x ", U_SHORT_VALUE(addr + i));
			break;
		case 4:
			gdbout("%08x ", U_LONG_VALUE(addr + i));
			break;
		case 8:
			gdbout("%016llx ", U_LONG_LONG_VALUE(addr + i));
			break;
		}
		if(i % 16 == 16 - width)
			gdbout("\n");
	}
	if(i % 16)
		gdbout("\n");
	return 0;
}

static int step_rtl_into(int argc, char **argv, c_interpreter *cptr)
{
	return OK_GDB_STEP_RTL_INTO;
}

static int stepinto(int argc, char **argv, c_interpreter *cptr)
{
	return OK_GDB_STEPINTO;
}

static int stepover(int argc, char **argv, c_interpreter *cptr)
{
	return OK_GDB_STEPOVER;
}

static int step_rtl_over(int argc, char **argv, c_interpreter *cptr)
{
	return OK_GDB_STEP_RTL_OVER;
}

int gdb::breakpoint(int argc, char **argv, c_interpreter *cptr)
{
	function_info *fptr;
	int line;
	if(argc < 2) {
		gdbout("Argument not enough\n");
		return ERROR_GDB_ARGC;
	}
	if(argc >= 3)
		line = katoi(argv[2]);
	else
		line = 0;
	//if(!kstrcmp(argv[1], "main")) {
	//	for(int i=0; i<cptr->nonseq_info->row_num; i++)
	//		kprintf("%03d %s\n", i, cptr->nonseq_info->row_info_node[i].row_ptr);
	//	return ERROR_NO;
	//}
	fptr = cptr->function_declare->find(argv[1]);
	if(!fptr)
		gdbout("Function not found.\n");
	else {
		mid_code *mid_code_base = (mid_code*)fptr->mid_code_stack.get_base_addr();
		if(line < 0 || line > fptr->row_line) {
			gdbout("Line not exist.\n");
			return ERROR_NO;
		}
		if((mid_code_base + fptr->row_code_ptr[line])->break_flag & BREAKPOINT_REAL) {
			gdbout("Breakpoint duplicated.\n");
			return ERROR_NO;
		}
		gdbout("Breakpoint set @ 0x%X\n", mid_code_base + fptr->row_code_ptr[line]);
		(mid_code_base + fptr->row_code_ptr[line])->break_flag |= BREAKPOINT_REAL;
		bp_info_t *bp_info_ptr = (bp_info_t*)dmalloc(sizeof(bp_info_t), "breakpoint info");
		node *bpnode_ptr = (node*)dmalloc(sizeof(node), "breakpoint node");
		bpnode_ptr->value = bp_info_ptr;
		bp_info_ptr->no = ++bp_info_ptr->total_no;
		bp_info_ptr->bp_addr = mid_code_base + fptr->row_code_ptr[line];
		bp_stack.push(bpnode_ptr);
		//vfree(bpnode_ptr);
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
	for(nptr=bp_stack.get_head()->right, i=0; i<bp_stack.get_count(); i++, nptr=nptr->right) {
		if(((bp_info_t*)nptr->value)->no == no) {
			gdbout("Delete breakpoint %d @ 0x%x.\n", no, (mid_code*)((bp_info_t*)nptr->value)->bp_addr);
			bp_stack.del(nptr);
			((mid_code*)((bp_info_t*)nptr->value)->bp_addr)->break_flag &= ~BREAKPOINT_REAL;
			vfree(nptr->value);
			vfree(nptr);
			return ERROR_NO;
		}
	}
	gdbout("Breakpoint not exist.\n");
	return ERROR_NO;
}

int info_ask(int argc, char **argv, c_interpreter *cptr)
{
	if(!kstrcmp(argv[1], "breakpoint")) {
		int i;
		node *nptr;
		for(nptr=bp_stack.get_head()->right, i=0; i<bp_stack.get_count(); i++, nptr=nptr->right) {
			gdbout("breakpoint @ 0x%X, id = %d\n", ((bp_info_t*)nptr->value)->bp_addr, ((bp_info_t*)nptr->value)->no);
		}
	}
	return ERROR_NO;
}

int gdb::print_code(int argc, char **argv, c_interpreter *cptr)
{
	function_info *fptr, *c_fptr;
	int line, i;
	char space_flag = 0;
	cptr->find_fptr_by_code(cptr->pc, c_fptr, &line);
	if(argc > 1) {
		fptr = cptr->function_declare->find(argv[1]);
		if(!kstrcmp(argv[1], "main")) {
			if(!fptr)
				for(i=0; i<cptr->nonseq_info->row_num; i++)
					gdbout("%03d %s\n", i, cptr->nonseq_info->row_info_node[i].row_ptr);
			else
				goto print_c;
		} else {
			if(!fptr)
				gdbout("Function not found.\n");
			else
				goto print_c;
		}
	} else {
		fptr = c_fptr;
print_c:
		if(fptr == c_fptr)
			space_flag = 1;
		for(i=0; i<fptr->row_line; i++) {
			if(space_flag)
				if(i == line)
					gdbout(" => ");
				else
					gdbout("    ");
			fptr->print_line(i);
		}
	}
	return ERROR_NO;
}

int gdb::print_mid_code(int argc, char **argv, c_interpreter *cptr)
{
	function_info *fptr, *c_fptr;
	int line, flag = 0;
	int begin_row, end_row, offset;
	cptr->find_fptr_by_code(cptr->pc, c_fptr, &line);
	if(argc > 1) {
		fptr = cptr->function_declare->find(argv[1]);
		if(!kstrcmp(argv[1], "main")) {
			if(fptr)
				goto print_c;
			else
				cptr->print_code((mid_code*)cptr->interprete_need_ptr->mid_code_stack.get_base_addr(), cptr->interprete_need_ptr->mid_code_stack.get_count(), 1);
		} else {
			if(!fptr)
				gdbout("Function not found.\n");
			else {
				goto print_c;
			}
		}
	} else {
		fptr = c_fptr;
print_c:
		if(fptr == c_fptr) {
			flag = 1;
			if(gdb::print_cfg == PRINT_CFG_NEAR) {
				offset = cptr->pc - (mid_code*)fptr->mid_code_stack.get_base_addr();
				begin_row = offset > gdb::near_rows ? offset - gdb::near_rows : 0;
				end_row = fptr->mid_code_stack.get_count() - offset  - 1 > gdb::near_rows ? offset + gdb::near_rows + 1 : fptr->mid_code_stack.get_count() - 1;
			} else {
				begin_row = 0;
				offset = cptr->pc - (mid_code*)fptr->mid_code_stack.get_base_addr();
				end_row = fptr->mid_code_stack.get_count() - 1;
			}
		} else {
			begin_row = 0;
			offset = 0;
			end_row = fptr->mid_code_stack.get_count() - 1;
		}
		cptr->print_code((mid_code*)fptr->mid_code_stack.get_base_addr() + begin_row, 
			end_row - begin_row + 1, 
			begin_row << PCODE_BEGIN_BIT | offset - begin_row << PCODE_ROW_BIT | flag << PCODE_NARROW_BIT | 1);
	}
	return ERROR_NO;
}

int gdb::config(int argc, char **argv, c_interpreter *cptr)
{
	if(argc < 2)
		return ERROR_NO;
	if(!kstrcmp(argv[1], "print")) {
		if(argc < 3)
			error("cfg print arg too few\n");
		if(!kstrcmp(argv[2], "all"))
			gdb::print_cfg = PRINT_CFG_ALL;
		else
			gdb::print_cfg = PRINT_CFG_NEAR;
	}
	return ERROR_NO;
}

cmd_t cmd_tab[] = {
	{"print", gdb::print},
	{"p", gdb::print},
	{"b", gdb::breakpoint},
	{"c", continue_exec},
	{"d", del_breakpoint},
	{"n", stepover},
	{"s", stepinto},
	{"set", gdbset},
	{"ni", step_rtl_over},//TODO: ni not step into
	{"si", step_rtl_into},
	{"info", info_ask},
	{"list", gdb::print_code},
	{"x", gdb::memory_dump},
	{"pmcode", gdb::print_mid_code},
	{"bt", print_call_stack},
	{"trace", gdb::trace_ctl},
	{"cfg", gdb::config},
};

int gdb::parse(char *cmd_str)
{
	int char_index = 0;
	int char_total_index = 0;
	char last_char = ' ';
	bool inword = false;
	argc = -1;
	char ch;
	kmemset(argstr, 0, sizeof(argstr));
	while(*cmd_str)
	{
		ch = *cmd_str++;
		if(ch == '/' && IsSpace(last_char)) {
			wptr[char_index] = 0;
			wptr = argstr;
			char_index = 0;
			inword = true;
		} else if(!IsSpace(ch) && IsSpace(last_char)) {
			if(argc >= 0) {
				char_total_index++;
				if(char_total_index > ARG_SPACE_SIZE) {
					gdbout("Cmd error: cmd too long\r\n");
					return -3;
				}
				wptr[char_index] = 0;
			}
			char_index = 0;
			argc++;
			if(argc >= MAX_GDBCMD_ARGC) {
				gdbout("Cmd error: too many args\r\n");
				return -2;
			}
			argv[argc] = &args[char_total_index];
			wptr = argv[argc];
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
			wptr[char_index++] = ch;
		}
		last_char = ch;
	}
	if(argc >= 0)
		argv[argc][char_index] = 0;
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
	gdbout("Cmd not found: %s\r\n", argv[0]);
	return -1;
}

char* gdb_complete(char *str, int times)
{
	char *arg1 = str;
	int tiplen = kstrlen(str);
	int i, j, k = 0, ret;
	int count = get_argc(str);
	const int cmd_count = sizeof(cmd_tab) / sizeof(cmd_t);
	while(*arg1 && kisspace(*arg1))
		arg1++;
	if(count == 1) {
		tiplen = kstrlen(arg1);
		for(j=0; j<cmd_count; j++)
			if(!kstrncmp(cmd_tab[j].str, str, tiplen)) {
				if(++k == times)
					return (char*)cmd_tab[j].str + tiplen;
			}
	} else if(count == 2) {

	}
	return 0;
}

int gdb::breakpoint_handle(c_interpreter *interpreter_ptr, mid_code *instruction_ptr)
{
	int gdbret = 0;
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
		if(line >= 0)
			gdbout("Running to function %s line %d.\n", (fptr ? fptr->get_name() : "main"), line);
		else
			warning("No debug information for function.\n");
		while(1) {
			instruction_ptr->break_flag &= ~BREAKPOINT_STEP;
			gdbout("gdb>");
			interpreter_ptr->tty_used->readline(gdbstr, '\t', gdb_complete);
			gdb::parse(gdbstr);
			gdbret = gdb::exec(interpreter_ptr);
			if(gdbret == OK_GDB_RUN || gdbret == OK_GDB_STEP_RTL_INTO || gdbret == OK_GDB_STEPINTO || gdbret == OK_GDB_STEPOVER || gdbret == OK_GDB_STEP_RTL_OVER) {
				switch(gdbret) {
				case OK_GDB_STEPOVER://n
					if(fptr && line != fptr->row_line - 1) {
						bp_todo = (mid_code*)fptr->mid_code_stack.get_base_addr() + fptr->row_code_ptr[line + 1] - 1;
					} else {
						if(fptr)
							bp_todo = (mid_code*)fptr->mid_code_stack.get_current_ptr() - 1;
					}
					break;
				case OK_GDB_STEP_RTL_OVER://ni

					break;
				case OK_GDB_STEPINTO://s
					break;
				case OK_GDB_STEP_RTL_INTO://si
					bp_todo = instruction_ptr;
					break;
				}
				break;
			}
		}
		return gdbret;
	}
}
#endif
