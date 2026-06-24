#pragma once

typedef unsigned int size_t;

void kmalloc_init();

void* kmalloc(size_t size);
void  kfree(void* ptr);