#pragma once
#ifndef KMALLOC_H
#define KMAALOC_H

void* kmalloc(unsigned int size);
int kfree(void* ptr);
void* krealloc(void *ptr, uint size);
void heapinit(void);

#endif
