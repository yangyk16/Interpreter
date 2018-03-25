#pragma once
#ifndef CSTDLIB_H
#define CSTDLIB_H

#include <stdarg.h>

#define NULL 0
int kprintf(const char *fmt, ...);
int ksprintf(char *buf, const char *fmt, ...);
void* kmemcpy(void *d, const void *s, unsigned int size);
void* kmemmove(void *d, const void *s, unsigned int size);
void* kmemset(void *d, int ch, unsigned int size);
unsigned int kstrlen(const char *str);
int kmemcmp(void *mem1, void *mem2, unsigned int size);
int kstrcmp(const char *str1, const char *str2);
int kstrncmp(const char *str1, const char *str2, unsigned int size);
char *kstrcpy(char *d, const char *s);
int katoi(const char* sptr);
double katof(const char* sptr);
bool kisdigit(unsigned char ch);
bool kisupper(unsigned char ch);
bool kislower(unsigned char ch);

void *vmalloc(unsigned int size);
void vfree(void*);
void* vrealloc(void* addr, unsigned int size);
#endif
