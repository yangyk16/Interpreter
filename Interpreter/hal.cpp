#include "hal.h"
#include <iostream>
using namespace std;
int tty::readline(char* str) {
	char ch;
	int i = 0;
	while(ch = getchar()) {
		if(ch == '\n')
			break;
		if(ch != '\r') {
			str[i++] = ch;
		}
	}
	return i;
};
