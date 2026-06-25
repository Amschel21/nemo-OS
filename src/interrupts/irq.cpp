#include "irq.hpp"

#include "../drivers/timer.hpp"
#include "../arch/x86/io.hpp"
#include "../ipc/sleep.hpp"

extern "C" void irq_handler()
{
    timer_ticks++;

    sleep_tick();

    outb(0x20, 0x20);
}