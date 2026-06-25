#include "../kernel/panic.hpp"
#include "../terminal.hpp"
#include "../libk/itoa.hpp"

static char pf_buf[16];

static void pf_print(const char* label, uint32_t v)
{
    itoa(v, pf_buf);
    terminal.write(label);
    terminal.write(pf_buf);
    terminal.write("\n");
}

extern "C" {
    uint32_t pf_err;
}

extern "C" void page_fault_handler()
{
    uint32_t err = pf_err;
    uint32_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));

    terminal.write("PAGE FAULT\n");
    pf_print(" err=", err);
    pf_print(" cr2=", cr2);

    PANIC("PAGE FAULT");
}