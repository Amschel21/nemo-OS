#include "irq.hpp"
#include "../drivers/timer.hpp"
#include "../arch/x86/io.hpp"

extern "C" void irq_handler()
{
    timer_ticks++;

    outb(0x20, 0x20);
}