#include "hal.h"
#include "config.h"
#include "kmalloc.h"
#include "cstdlib.h"
#include "error.h"

#if TTY_TYPE == 0
#include <iostream>
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

int kfputs(char *str)
{
#if TTY_TYPE == 0
	printf("%s", str);
#else
	uart_sendstring(str);
#endif
	return 0;
}

void *kfopen(char *filename, char *mode)
{
#if TTY_TYPE == 0
	return fopen(filename, mode);
#endif
}

int kfclose(void *fp)
{
#if TTY_TYPE == 0
	return fclose((FILE*)fp);
#endif
}

unsigned int kfread(void *buffer, unsigned int size, unsigned int nmemb, void *fileptr)
{
#if TTY_TYPE == 0
	return fread(buffer, size, nmemb, (FILE*)fileptr);
#endif
}

unsigned int kfwrite(void *buffer, unsigned int size, unsigned int count, void *fileptr)
{
#if TTY_TYPE == 0
	return fwrite(buffer, size, count, (FILE*)fileptr);
#endif
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
