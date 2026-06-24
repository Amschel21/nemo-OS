#include "syscall.hpp"

#include "../drivers/timer.hpp"

extern "C"
unsigned int syscall_dispatch(
    unsigned int eax,
    unsigned int ebx,
    unsigned int ecx,
    unsigned int edx)
{
    (void)ebx;
    (void)ecx;
    (void)edx;

    switch(eax)
    {
        case 1:
            return timer_ticks;

        default:
            return 0xFFFFFFFF;
    }
}