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
#include "memory/vmm.hpp"
#include "fs/ramfs.hpp"
#include "scheduler/task.hpp"
#include "scheduler/scheduler.hpp"
#include "drivers/timer_ticks.hpp"
#include "kernel/panic.hpp"
#include "interrupts/tss.hpp"
#include "process/process.hpp"
#include "libk/itoa.hpp"
#include "user/user_mode.hpp"
#include "ipc/sleep.hpp"

extern "C" void shell_task();

static void shell_task_main()
{
    terminal.write("[shell] up\n");

    asm volatile("sti");

    while(1)
    {
        shell_tick();
        asm volatile("hlt");
    }
}

extern "C" void shell_task()
{
    shell_task_main();
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

    multiboot_init((multiboot_info_t*)addr);

    if(total_memory_mb == 0)
    {
        PANIC("Memory detection failed");
    }

    pmm_init(total_memory_mb);

    paging_init();

    vmm_init();

    kmalloc_init();

    ramfs_init();

    gdt_init();

    tss_init();

    idt_init();

    pic_remap();

    timer_init(100);

    sleep_init();

    terminal.write("NemoOS v1.0\n");

    shell_init();

    scheduler_init();

    Process* shell_proc =
        process_create_kernel(shell_task);

    if(!shell_proc)
    {
        terminal.write(
            "kernel: process creation failed\n");

        while(1)
        {
            asm volatile("hlt");
        }
    }

    terminal.write(
        "[kernel] system ready\n");

    asm volatile("sti");

    process_first_enter(shell_proc);

    while(1)
    {
        asm volatile("hlt");
    }
}