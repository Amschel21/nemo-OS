#include "idt.hpp"
#include "isr.hpp"

extern "C" void idt_flush(unsigned int);

extern "C" void irq0();
extern "C" void irq1();
extern "C" void page_fault_stub();
extern "C" void syscall_stub();

static IDTEntry idt[256];
static IDTPtr idt_ptr;

static void idt_set_gate(
    uint8_t num,
    uint32_t base,
    uint16_t sel,
    uint8_t flags)
{
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;

    idt[num].selector = sel;
    idt[num].always0  = 0;
    idt[num].flags    = flags;
}

void idt_init()
{
    idt_ptr.limit =
        sizeof(IDTEntry) * 256 - 1;

    idt_ptr.base =
        (uint32_t)&idt;

    for(int i = 0; i < 256; i++)
    {
        idt[i].base_low = 0;
        idt[i].base_high = 0;
        idt[i].selector = 0;
        idt[i].always0 = 0;
        idt[i].flags = 0;
    }

    idt_set_gate(
        0,
        (uint32_t)isr0,
        0x08,
        0x8E);

    idt_set_gate(
        32,
        (uint32_t)irq0,
        0x08,
        0x8E);
    
    idt_set_gate(
        33,
        (uint32_t)irq1,
        0x08,
        0x8E);
    
    idt_set_gate(
    14,
    (uint32_t)page_fault_stub,
    0x08,
    0x8E);

    idt_set_gate(
    128,
    (uint32_t)syscall_stub,
    0x08,
    0xEE);

    idt_flush((uint32_t)&idt_ptr);
}