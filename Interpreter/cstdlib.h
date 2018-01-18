#pragma once
#ifndef CSTDLIB_H
#define CSTDLIB_H

#include <stdarg.h>

int kprintf(const char *fmt, ...);
void* kmemcpy(void *d, const void *s, unsigned int size);
void* kmemset(void *d, int ch, unsigned int size);
int ksprintf(char *buf, const char *fmt, va_list args);
unsigned int kstrlen(const char *str);
int kstrcmp(const char *str1, const char *str2);
char *kstrcpy(char *d, const char *s);
#endif
