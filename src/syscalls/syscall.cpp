#include "syscall.hpp"

#include "../drivers/timer.hpp"
#include "../terminal.hpp"
#include "../drivers/keyboard_buffer.hpp"

extern "C"
unsigned int syscall_dispatch(
    unsigned int eax,
    unsigned int ebx,
    unsigned int ecx,
    unsigned int edx)
{
    (void)edx;

    switch(eax)
    {
        case 1:
            return timer_ticks;

        case 2:
        {
            const char* str = (const char*)ebx;

            if(!str)
                return 0;

            unsigned int len = ecx;

            for(unsigned int i = 0; i < len; i++)
            {
                if(!str[i])
                    break;

                terminal.putchar(str[i]);
            }

            return len;
        }

        case 3:
        {
            char* buf = (char*)ebx;

            if(!buf || ecx == 0)
                return 0;

            unsigned int max = ecx;
            unsigned int count = 0;

            while(count < max)
            {
                char c;

                if(keyboard_buffer_pop(&c))
                {
                    buf[count++] = c;
                }
                else
                {
                    break;
                }
            }

            return count;
        }

        default:
            return 0xFFFFFFFF;
    }
}