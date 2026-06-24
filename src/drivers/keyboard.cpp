#include "keyboard.hpp"
#include "../arch/x86/ports.hpp"

#define SC_SHIFT_L     0x2A
#define SC_SHIFT_R     0x36
#define SC_CTRL_L      0x1D
#define SC_ALT_L       0x38
#define SC_CAPS        0x3A
#define SC_ENTER       0x1C
#define SC_BACKSPACE   0x0E
#define SC_TAB         0x0F
#define SC_ESC         0x01
#define SC_SPACE       0x39

static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static bool caps_lock = false;

static const char lower[128] =
{
    0,          // 0x00
    27,         // 0x01 Esc
    '1','2','3','4','5','6','7','8','9','0',  // 0x02-0x0B
    '-','=',    // 0x0C-0x0D
    '\b',       // 0x0E Backspace
    '\t',       // 0x0F Tab
    'q','w','e','r','t','y','u','i','o','p',  // 0x10-0x19
    '[',']',    // 0x1A-0x1B
    '\n',       // 0x1C Enter
    0,          // 0x1D Ctrl
    'a','s','d','f','g','h','j','k','l',      // 0x1E-0x26
    ';','\'','`',// 0x27-0x29
    0,          // 0x2A Shift
    '\\',       // 0x2B
    'z','x','c','v','b','n','m',              // 0x2C-0x32
    ',','.','/',// 0x33-0x35
    0,          // 0x36 Shift
    '*',        // 0x37
    0,          // 0x38 Alt
    ' ',        // 0x39 Space
    0,          // 0x3A Caps
    0,0,0,0,0,0,0,0,0,0,  // 0x3B-0x44 F1-F10
    0,          // 0x45 NumLock
    0,          // 0x46 ScrollLock
    0,0,0,0,0,0,0,  // 0x47-0x4D Home..PgDn
    0,          // 0x4E
    0,          // 0x4F End
    0,          // 0x50
    0,0,0,0,0,  // 0x51-0x55
    0,          // 0x56
    0,0         // 0x57-0x58 F11-F12
};

static const char upper[128] =
{
    0,
    27,
    '!','@','#','$','%','^','&','*','(',')',
    '_','+',
    '\b',
    '\t',
    'Q','W','E','R','T','Y','U','I','O','P',
    '{','}',
    '\n',
    0,
    'A','S','D','F','G','H','J','K','L',
    ':','"','~',
    0,
    '|',
    'Z','X','C','V','B','N','M',
    '<','>','?',
    0,
    '*',
    0,
    ' ',
    0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,
    0,0,0,0,0,0,0,
    0,0,0,0,0,
    0,
    0,0
};

char keyboard_scancode_to_ascii(unsigned char scancode)
{
    if(scancode & 0x80)
    {
        unsigned char make = scancode & 0x7F;

        if(make == SC_SHIFT_L || make == SC_SHIFT_R)
            shift_pressed = false;

        if(make == SC_CTRL_L)
            ctrl_pressed = false;

        if(make == SC_ALT_L)
            alt_pressed = false;

        return 0;
    }

    if(scancode == SC_CAPS)
    {
        caps_lock = !caps_lock;
        return 0;
    }

    if(scancode == SC_SHIFT_L || scancode == SC_SHIFT_R)
    {
        shift_pressed = true;
        return 0;
    }

    if(scancode == SC_CTRL_L)
    {
        ctrl_pressed = true;
        return 0;
    }

    if(scancode == SC_ALT_L)
    {
        alt_pressed = true;
        return 0;
    }

    if(scancode >= 128)
        return 0;

    bool shift = shift_pressed || caps_lock;

    return shift ? upper[scancode] : lower[scancode];
}

void keyboard_init()
{
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
    caps_lock = false;
}
