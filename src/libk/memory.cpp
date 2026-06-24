#include "memory.hpp"

void* memcpy(void* dest, const void* src, unsigned int size)
{
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;

    for(unsigned int i = 0; i < size; i++)
        d[i] = s[i];

    return dest;
}

void* memset(void* dest, int value, unsigned int size)
{
    unsigned char* d = (unsigned char*)dest;

    for(unsigned int i = 0; i < size; i++)
        d[i] = (unsigned char)value;

    return dest;
}