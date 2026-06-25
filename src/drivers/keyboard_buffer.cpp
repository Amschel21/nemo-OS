#include "keyboard_buffer.hpp"

static char buffer[256];
static int head = 0;
static int tail = 0;

void keyboard_buffer_push(char c)
{
    int next = (head + 1) % 256;

    if(next == tail)
        return;

    buffer[head] = c;
    head = next;
}

bool keyboard_buffer_pop(char* c)
{
    unsigned long flags;
    asm volatile("pushfl; popl %0; cli" : "=r"(flags));

    if (tail == head)
    {
        asm volatile("pushl %0; popfl" : : "r"(flags));
        return false;
    }

    *c = buffer[tail];
    tail = (tail + 1) % 256;

    asm volatile("pushl %0; popfl" : : "r"(flags));
    return true;
}