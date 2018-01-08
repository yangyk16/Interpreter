#pragma once
#ifndef KMALLOC_H
#define KMAALOC_H

void* kmalloc(unsigned int size);
int kfree(void* ptr);
void heapinit(void);

#endif
