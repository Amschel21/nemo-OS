#include "tss.hpp"
#include "gdt.hpp"

extern "C" void tss_flush();
extern "C" uint32_t stack_top;

static TSSEntry tss;

extern void gdt_set_gate(
    int num,
    uint32_t base,
    uint32_t limit,
    uint8_t access,
    uint8_t gran);

void tss_set_kernel_stack(uint32_t stack)
{
    tss.esp0 = stack;
}

void tss_init()
{
    uint32_t base =
        (uint32_t)&tss;

    uint32_t limit =
        sizeof(TSSEntry);

    gdt_set_gate(
        5,
        base,
        limit,
        0x89,
        0x40);

    tss.ss0 = 0x10;

    tss.esp0 =
        (uint32_t)&stack_top;

    tss.cs = 0x0B;
    tss.ss = 0x13;
    tss.ds = 0x13;
    tss.es = 0x13;
    tss.fs = 0x13;
    tss.gs = 0x13;

    tss.iomap_base =
        sizeof(TSSEntry);

    tss_flush();
}