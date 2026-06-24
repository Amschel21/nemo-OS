#include "itoa.hpp"

void itoa(unsigned int value, char* buffer)
{
    char temp[16];
    int i = 0;

    if(value == 0)
    {
        buffer[0] = '0';
        buffer[1] = 0;
        return;
    }

    while(value > 0)
    {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    }

    int j = 0;

    while(i > 0)
    {
        buffer[j++] = temp[--i];
    }

    buffer[j] = 0;
}