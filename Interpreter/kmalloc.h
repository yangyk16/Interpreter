#pragma once
#ifndef KMALLOC_H
#define KMAALOC_H

#ifdef __cplusplus
extern "C" {
#endif

void* kmalloc(unsigned int size);
int kfree(void* ptr);
void* krealloc(void *ptr, unsigned int size);
void heapinit(void);
void heap_debug(void);

#ifdef __cplusplus
}
#endif
#endif
