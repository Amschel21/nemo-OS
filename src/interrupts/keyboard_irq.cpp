#include "../arch/x86/io.hpp"
#include "../drivers/keyboard.hpp"
#include "../drivers/keyboard_buffer.hpp"

extern "C" void keyboard_irq_handler()
{
    unsigned char sc = inb(0x60);

    char c = keyboard_scancode_to_ascii(sc);

    if (c != 0) {
        keyboard_buffer_push(c);
    }

    outb(0x20, 0x20);
}