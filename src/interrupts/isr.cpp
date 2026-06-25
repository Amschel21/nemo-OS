#include "isr.hpp"
#include "../terminal.hpp"
#include "../arch/x86/ports.hpp"
#include "../libk/itoa.hpp"

extern "C" unsigned int default_vector;

extern "C" void default_interrupt_handler()
{
    char buf[16];
    terminal.write("INT? vec=");
    itoa(default_vector, buf);
    terminal.write(buf);
    terminal.write("\n");
    outb(0x20, 0x20);
}

void isr_install()
{
}