#include "hal.h"
#include "config.h"
#include "kmalloc.h"
#include "cstdlib.h"
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
extern "C" void uart_getstring(char *str);
int uart::readline(char* str)
{
	uart_getstring(str);
	return 0;
};
extern "C" void UartSendString(char * src );
int uart::puts(char* str)
{
	UartSendString(str);
	return 0;
}
#endif

void* vmalloc(unsigned int size)
{
	unsigned int a_size = size&7?size+(8-(size&7)):size;
	void* ret = kmalloc(a_size);
	if(ret) {
		//size = size&7?size+(8-size&7):size;
		debug("malloc %x, %d\n", ret, size);
		kmemset(ret, 0, size);
		return ret;
	} else {
		return NULL;
	}
}

void vfree(void *ptr)
{
	kfree(ptr);
	debug("free %x\n", ptr);
}

void* vrealloc(void* addr, unsigned int size)
{
	debug("realloc %x, %d\n", addr, size);
	return krealloc(addr, size);
}

int kfputs(char *str)
{
#if TTY_TYPE == 0
	printf("%s", str);
#else
	UartSendString(str);
#endif
	return 0;
}
