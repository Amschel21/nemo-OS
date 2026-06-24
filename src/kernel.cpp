#include "terminal.hpp"
#include "interrupts/gdt.hpp"
#include "interrupts/idt.hpp"
#include "interrupts/pic.hpp"
#include "drivers/timer.hpp"
#include "shell/shell.hpp"
#include "boot/multiboot.hpp"
#include "memory/pmm_bitmap.hpp"
#include "memory/kmalloc.hpp"
#include "memory/paging.hpp"
#include "fs/ramfs.hpp"
#include "scheduler/task.hpp"
#include "scheduler/scheduler.hpp"
#include "drivers/timer_ticks.hpp"
#include "kernel/panic.hpp"
#include "interrupts/tss.hpp"

static unsigned int syscall_get_ticks()
{
    unsigned int result;

    asm volatile(
        "mov $1, %%eax \n"
        "int $0x80     \n"
        : "=a"(result)
        :
        : "memory"
    );

    return result;
}

extern "C" void kernel_main(
    unsigned int magic,
    unsigned int addr)
{
    terminal.init();

    if(magic != 0x2BADB002)
    {
        PANIC("Invalid multiboot magic");
    }

    multiboot_init(
        (multiboot_info_t*)addr);

    if(total_memory_mb == 0)
    {
        PANIC("Memory detection failed");
    }

    pmm_init(total_memory_mb);

    paging_init();

    kmalloc_init();

    ramfs_init();

    gdt_init();

    tss_init();

    idt_init();

    pic_remap();

    timer_init(100);

    terminal.write("NemoOS v0.9\n");

    unsigned int ticks =
    syscall_get_ticks();

terminal.write("Ticks: ");

char buf[16];

int pos = 0;

unsigned int n = ticks;

if(n == 0)
{
    buf[pos++] = '0';
}
else
{
    char tmp[16];
    int t = 0;

    while(n > 0)
    {
        tmp[t++] = '0' + (n % 10);
        n /= 10;
    }

    while(t > 0)
    {
        buf[pos++] = tmp[--t];
    }
}

buf[pos] = 0;

terminal.write(buf);
terminal.write("\n");

    terminal.write("Testing syscall...\n");

    asm volatile(
    "mov $0, %%eax\n"
    "int $0x80\n"
    :
    :
    : "eax"
);

    asm volatile(
    "mov $1, %%eax\n"
    "int $0x80\n"
    :
    :
    : "eax"
);

    asm volatile("sti");

    shell_init();

    scheduler_init();

   

    while(true)
    {
        shell_tick();

        scheduler_run();

        asm volatile("hlt");
    }
}