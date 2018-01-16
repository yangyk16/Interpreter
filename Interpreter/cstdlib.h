#pragma once
#ifndef CSTDLIB_H
#define CSTDLIB_H

int kprintf(const char *fmt, ...);
void* kmemcpy(void *d, void *s, unsigned int size);
void* kmemset(void *d, int ch, unsigned int size);
int vsprintf(char *buf, const char *fmt, va_list args);
#endif
