#include "keyboard_buffer.hpp"

static char buffer[256];
static int head = 0;
static int tail = 0;

void keyboard_buffer_push(char c)
{
    buffer[head] = c;
    head = (head + 1) % 256;
}

bool keyboard_buffer_pop(char* c)
{
    if (tail == head)
        return false;

    *c = buffer[tail];
    tail = (tail + 1) % 256;
    return true;
}