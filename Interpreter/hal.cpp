#include "hal.h"
#include "config.h"
#include "kmalloc.h"
#include "cstdlib.h"
#include "error.h"
#include "interpreter.h"

#if TTY_TYPE == 0
#include <iostream>
#include <stdio.h>
using namespace std;
int tty::readline(char* str)//×îºó±ØÐë²¹0
{
	char ch;
	int i = 0;
	while(ch = getchar()) {
		if(ch == '\n')
			break;
		if(ch != '\r') {
			str[i++] = ch;
		}
	}
	str[i] = 0;
	return i;
};

int tty::puts(char* str)
{
	cout << str;
	return 0;
}
#elif TTY_TYPE == 1
#include "../testcase/ff.h"
extern "C" int uart_getstring(char *str);
extern "C" void uart_sendstring(char *str);
int uart::readline(char* str)
{
	int i = uart_getstring(str);
	return i;
};

int uart::puts(char* str)
{
	uart_sendstring(str);
	return 0;
}
#endif

int file::init(char *filename)
{
	file_ptr = kfopen(filename, "r");
	if(file_ptr)
		return 0;
	else
		return -1;
}

void file::dispose(void)
{
	vfree(file_ptr);
}

int file::readline(char *str)
{
	int i = 0;
	int len;
	while(len = kfread(str + i, 1, 1, file_ptr)) {
		if((str[i] == 0) | (str[i] == '\n'))
			break;
		if(str[i] == '\r')
			continue;
		i++;
	}
	if(len == 0 && i == 0)
		return -1;
	str[i] = 0;
	return i;
}

int kfputs(char *str)
{
#if TTY_TYPE == 0
	printf("%s", str);
#else
	uart_sendstring(str);
#endif
	return 0;
}

void *kfopen(const char *filename, const char *mode)
{
#if TTY_TYPE == 0
	return fopen(filename, mode);
#else
	int ret;
	int tmode = 0;
	for(int i=0; mode[i]; i++) {
		if(mode[i] == 'r')
			tmode |= FA_READ; 
		if(mode[i] == 'w')
			tmode |= FA_WRITE | FA_CREATE_ALWAYS; 
	}
	FIL *file = (FIL*)dmalloc(sizeof(FIL), "");
	ret = f_open (file, filename, tmode);	/* Open or create a file */
	if(!ret)
		return file;
	else {
		vfree(file);
		return 0;
	}
#endif
}

int kfclose(void *fp)
{
#if TTY_TYPE == 0
	return fclose((FILE*)fp);
#else
	vfree(fp);
	return f_close((FIL*)fp);
#endif
}

unsigned int kfread(void *buffer, unsigned int size, unsigned int nmemb, void *fileptr)
{
#if TTY_TYPE == 0
	return fread(buffer, size, nmemb, (FILE*)fileptr);
#else
	unsigned int ret;
	f_read((FIL*)fileptr, buffer, size * nmemb, &ret);
	return ret;
#endif
}

unsigned int kfwrite(void *buffer, unsigned int size, unsigned int count, void *fileptr)
{
#if TTY_TYPE == 0
	int ret;
	ret = fwrite(buffer, size, count, (FILE*)fileptr);
	fflush((FILE*)fileptr);
	return ret;
#else
	unsigned int ret;
	f_write((FIL*)fileptr, buffer, size * count, &ret);
	return ret;
#endif
}

void *irq_service[32];
void *irq_data[32];
extern c_interpreter irq_interpreter;
void irq_entry(int irq_no)
{
	stack *mid_code_stack = &((function_info*)irq_service[irq_no])->mid_code_stack;
	irq_interpreter.exec_mid_code((mid_code*)mid_code_stack->get_base_addr(), mid_code_stack->get_count());
}

void irq_reg(int irq_no, void *func_ptr, void *data)
{
	irq_service[irq_no] = func_ptr;
	irq_data[irq_no] = data;
	//intreg(irq_no, irq_entry, 0);
}

int hard_fault_check(int addr)
{
#if TTY_TYPE == 1
	if(addr < 0x200000 || addr >= 0x14000000 || (addr > 0x37FFFF && addr < 0x2000000) || (addr > 0x2400000 && addr <= 0x10000000)) {
		return ERROR_HARD_FAULT;
	}
#else
#endif
	return ERROR_NO;
}
