#pragma once
#ifndef CSTDLIB_H
#define CSTDLIB_H

#include <stdarg.h>
#include "config.h"

#ifndef NULL
#define NULL 0
#endif

#ifndef WIN32
#define GREEN_SEQ "\033[31m"
#define YELLOW_SEQ "\033[33m"
#define BLACK_SEQ "\033[0m"
#else
#define GREEN_SEQ
#define YELLOW_SEQ
#define BLACK_SEQ
#endif

#define PRINT_LEVEL_DEBUG	6
#define PRINT_LEVEL_WARNING	5
#define PRINT_LEVEL_ERROR	4
#define PRINT_LEVEL_FATAL	3
#define PRINT_LEVEL_TIP		1
#define PRINT_LEVEL_GDBOUT	1
#define PRINT_LEVEL_MASKALL	0

#if INTERPRETER_DEBUG
#define PRINT_LEVEL_DEFAULT	PRINT_LEVEL_DEBUG
#else
#define PRINT_LEVEL_DEFAULT	PRINT_LEVEL_WARNING
#endif
#ifdef __cplusplus
extern "C" {
#endif
int kprintf(const char *fmt, ...);
int kprintf_l(int level, const char *fmt, ...);
void set_print_level(int level);
int ksprintf(char *buf, const char *fmt, ...);
void* kmemcpy(void *d, const void *s, unsigned int size);
void* kmemmove(void *d, const void *s, unsigned int size);
void* kmemset(void *d, int ch, unsigned int size);
int kmemchk(void *s, char ch, unsigned int size);
unsigned int kstrlen(const char *str);
int kmemcmp(void *mem1, void *mem2, unsigned int size);
int kstrcmp(const char *str1, const char *str2);
int kstrncmp(const char *str1, const char *str2, unsigned int size);
char *kstrcpy(char *d, const char *s);
int katoi(const char* sptr);
double katof(const char* sptr);
int kisdigit(int ch);
int kisupper(int ch);
int kislower(int ch);
int kisalnum(int ch);
#ifdef __cplusplus
}
#endif
void *dmalloc(unsigned int size, char *info);
void vfree(void*);
void* vrealloc(void* addr, unsigned int size);

#define debug(fmt, ...) kprintf_l(PRINT_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define tip(fmt, ...) kprintf_l(PRINT_LEVEL_TIP, fmt, ##__VA_ARGS__)
#define error(fmt, ...)	kprintf_l(PRINT_LEVEL_ERROR, GREEN_SEQ fmt BLACK_SEQ, ##__VA_ARGS__)
#define warning(fmt, ...) kprintf_l(PRINT_LEVEL_WARNING, YELLOW_SEQ fmt BLACK_SEQ, ##__VA_ARGS__)
#define gdbout(fmt, ...) kprintf_l(PRINT_LEVEL_GDBOUT, fmt, ##__VA_ARGS__)
#endif
