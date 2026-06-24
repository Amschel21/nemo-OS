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
    if (tail == head)
        return false;

    *c = buffer[tail];
    tail = (tail + 1) % 256;
    return true;
}