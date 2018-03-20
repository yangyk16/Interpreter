#include "hal.h"
#include "config.h"
#include "cstdlib.h"
#include "error.h"
#if TTY_TYPE == 0
#include <stdio.h>
#endif

#if TTY_TYPE == 0
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
	printf("%s", str);
	return 0;
}
#elif TTY_TYPE == 1
extern "C" int uart_getstring(char *str);
int uart::readline(char* str)
{
	return uart_getstring(str);
};
extern "C" void UartSendString(char * src );
int uart::puts(char* str)
{
	UartSendString(str);
	return 0;
}
#endif

int kfputs(char *str)
{
#if TTY_TYPE == 0
	printf("%s", str);
#else
	UartSendString(str);
#endif
	return 0;
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