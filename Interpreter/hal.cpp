#include "hal.h"
#include "config.h"
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

void* vmalloc(unsigned int size)
{
	unsigned int a_size = size&7?size+(8-(size&7)):size;
	void* ret = malloc(a_size);
	if(ret) {
		//size = size&7?size+(8-size&7):size;
		debug("malloc %x, %d\n", ret, size);
		memset(ret, 0, size);
		return ret;
	} else {
		return NULL;
	}
}

void vfree(void *ptr)
{
	free(ptr);
	debug("free %x\n", ptr);
}
