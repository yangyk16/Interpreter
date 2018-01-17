#pragma once
#ifndef CSTDLIB_H
#define CSTDLIB_H

int kprintf(const char *fmt, ...);
void* kmemcpy(void *d, const void *s, unsigned int size);
void* kmemset(void *d, int ch, unsigned int size);
int ksprintf(char *buf, const char *fmt, ...);
#endif
