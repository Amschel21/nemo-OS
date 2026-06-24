#include "keyboard.hpp"

static const char keymap[128] =
{
    0,
    27,
    '1','2','3','4','5','6','7','8','9','0',
    '\'',
    0,
    '\b',
    '\t',

    'q','w','e','r','t','y','u','i','o','p',

    0,0,
    '\n',

    0,

    'a','s','d','f','g','h','j','k','l',

    0,
    0,

    0,

    0,

    '\\',

    'z','x','c','v','b','n','m',
    ',', '.', '-',

    0,

    '*',

    0,

    ' ',

    0
};

char keyboard_scancode_to_ascii(unsigned char scancode)
{
    if(scancode >= 128)
        return 0;

    return keymap[scancode];
}

void keyboard_init()
{
}