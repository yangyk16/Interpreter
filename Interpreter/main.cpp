// Interpreter.cpp : 定义控制台应用程序的入口点。
//

#ifdef WIN32
#include "stdafx.h"
#include <windows.h>
#endif
#include "interpreter.h"
#include <stdlib.h>
#include <stdio.h>
#include "kmalloc.h"
#include <iostream>
#include <signal.h>
#include "getopt.h"
#include "error.h"
#include "toolchain.h"

using namespace std;

extern c_interpreter myinterpreter;
extern c_interpreter irq_interpreter;

void ouch(int sig)
{
	myinterpreter.set_break_flag(1);
	myinterpreter.print_call_stack();
}

void tch2ch(char *src)
{
	char *dest = src;
	for(; *src;) {
		*dest++ = *src;
		if(*(src + 1))
			*dest++ = *(src + 1);
		src += 2;
	}
	*dest = 0;
}

#define MAX_ARG_LEN             8
#define MAX_GDBCMD_ARGC 8
#define ARG_SPACE_SIZE  256

typedef struct cmdinfo_s {
        int argc;
        char argstr[MAX_ARG_LEN];
        char *argv[MAX_GDBCMD_ARGC];
        char args[ARG_SPACE_SIZE - sizeof(char*) * MAX_GDBCMD_ARGC];
        char *wptr;
} cmdinfo_t;

cmdinfo_t cmdinfo;
inline int IsSpace(char ch){return ch == ' ' || ch == '\t';}
int parse(char *cmd_str)
{
        int char_index;
        int char_total_index;
        char last_char;
        int inword;
        char ch;
begin:
        char_index = 0;
        char_total_index = 0;
        last_char = ' ';
        inword = false;
        cmdinfo.argc = -1;
        kmemset(cmdinfo.argstr, 0, sizeof(cmdinfo.argstr));
        while(*cmd_str)
        {
                ch = *cmd_str++;
                if(!IsSpace(ch) && IsSpace(last_char)) {
                        if(cmdinfo.argc >= 0) {
                                char_total_index++;
                                if(char_total_index > ARG_SPACE_SIZE) {
                                        error("Cmd error: cmd too long\r\n");
                                        return -3;
                                }
                                cmdinfo.argv[cmdinfo.argc][char_index] = 0;
                        }
                        char_index = 0;
                        cmdinfo.argc++;
                        if(cmdinfo.argc >= MAX_GDBCMD_ARGC) {
                                error("Cmd error: too many args\r\n");
                                return -2;
                        }
                        cmdinfo.argv[cmdinfo.argc] = &cmdinfo.args[char_total_index];
                        cmdinfo.wptr = cmdinfo.argv[cmdinfo.argc];
                        inword = true;
                }
                if(IsSpace(ch) && !IsSpace(last_char)) {
                        inword = false;
                }
                if(inword) {
                        char_total_index++;
                        if(char_total_index > ARG_SPACE_SIZE) {
                                error("Cmd error: cmd too long\r\n");
                                return -3;
                        }
                        cmdinfo.wptr[char_index++] = ch;
                }
                last_char = ch;
        }
        if(cmdinfo.argc >= 0)
                cmdinfo.argv[cmdinfo.argc][char_index] = 0;
        cmdinfo.argc++;
        return cmdinfo.argc;
}

typedef int (*cctool)(int argc, char **argv);
typedef struct tool_cmd_s {
	const char *cmdname;	
	const char *issue;
	cctool cmdptr;
} tool_cmd;

tool_cmd cmdset[] = {
	{"cc", "", cc},
	{"db", "", db},
	{"objdump", "", objdump},
};

int exec(void)
{
        static int cmd_count = sizeof(cmdset) / sizeof(tool_cmd);
        int i;
        if(cmdinfo.argc <= 0)return -1;
        for(i=0; i<cmd_count; i++)
                if(!kstrcmp(cmdset[i].cmdname, cmdinfo.argv[0]) && cmdset[i].cmdptr != 0) {
                        //uart_printf("Exec cmd: %s\n", cmdinfo.argv[0]);
                        return cmdset[i].cmdptr(cmdinfo.argc, cmdinfo.argv);
                }
        error("Wrong Cmd: %s\r\n", cmdinfo.argv[0]);
        return -1;
}

#ifdef WIN32
bool OpenANSIControlChar()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) 
	{ 
		return false;
	}
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
		return false;
	}
    dwMode |= 0x4;
    if (!SetConsoleMode(hOut, dwMode)) {
		return false;
	}
    return true;
}
#endif

char cmdstr[256];
#ifdef WIN32
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char *argv[])
#endif
{
	signal(SIGINT, ouch);
#ifdef WIN32
	for(int i=1; i<argc; i++)
		tch2ch((char*)argv[i]);
	int ret = OpenANSIControlChar();
#endif
	global_init();
	//for(int i=0; i<sizeof(cmdset)/sizeof(tool_cmd); i++) {
	//	if(!kstrcmp((char*)argv[1], cmdset[i].cmdname)) {
	//		cmdset[i].cmdptr(argc - 1, (char**)&argv[1]);
	//		break;
	//	}
	//}
	while(1) {
		koptrst();
		tip("sh> ");
		cin.getline(cmdstr, 256);
		if (!kstrcmp(cmdstr, "exit"))
			break;
		parse(cmdstr);
		exec();
	}
	global_dispose();
	error("no cmd.\n");
	getchar();
	return 0;
}
