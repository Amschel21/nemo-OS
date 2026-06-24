#include "kmalloc.hpp"

static unsigned int heap_start = 0x1000000; // 16 MB
static unsigned int heap_current = 0x1000000;

void kmalloc_init()
{
    heap_current = heap_start;
}

void* kmalloc(size_t size)
{
    void* ptr = (void*)heap_current;

    heap_current += size;

    if(heap_current & 0x3)
        heap_current = (heap_current + 4) & ~0x3;

    return ptr;
}

void kfree(void*)
{
    // stub
}