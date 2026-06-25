#include "sleep.hpp"

static volatile uint32_t sleep_ticks = 0;

void sleep_init()
{
    sleep_ticks = 0;
}

void sleep_tick()
{
    sleep_ticks++;
}

void sleep_ms(uint32_t ms)
{
    uint32_t target = sleep_ticks + ms / 10;

    while(sleep_ticks < target)
    {
        asm volatile("hlt");
    }
}