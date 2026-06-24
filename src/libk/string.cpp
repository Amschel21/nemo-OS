#include "string.hpp"

int strcmp(const char* a, const char* b)
{
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *(unsigned char*)a - *(unsigned char*)b;
}

int strncmp(const char* a, const char* b, unsigned int n)
{
    for (unsigned int i = 0; i < n; i++) {
        if (a[i] != b[i] || a[i] == 0 || b[i] == 0)
            return (unsigned char)a[i] - (unsigned char)b[i];
    }
    return 0;
}

char* strcpy(char* dst, const char* src)
{
    char* ret = dst;

    while(*src)
    {
        *dst++ = *src++;
    }

    *dst = '\0';

    return ret;
}