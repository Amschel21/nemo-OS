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
#include "fs/vfs.hpp"
#include "fs/ramfs.hpp"
#include "fs/fat32.hpp"
#include "fs/elf.hpp"
#include "fs/test_elf.h"
#include "fs/hello_elf.h"
#include "scheduler/task.hpp"
#include "scheduler/scheduler.hpp"
#include "drivers/timer_ticks.hpp"
#include "kernel/panic.hpp"
#include "interrupts/tss.hpp"
#include "process/process.hpp"
#include "libk/itoa.hpp"
#include "user/user_mode.hpp"
#include "ipc/sleep.hpp"
#include "ipc/ipc.hpp"
#include "drivers/ata.hpp"
#include "drivers/pci.hpp"
#include "drivers/rtl8139.hpp"
#include "net/net.hpp"

extern "C" void shell_task();

static void shell_task_main()
{
    terminal.write("[shell] up\n");

    asm volatile("sti");

    while(1)
    {
        shell_tick();
        net_poll();
        asm volatile("hlt");
    }
}

extern "C" void shell_task()
{
    shell_task_main();
}

// === Stress test tasks (scheduler validation) ===

volatile int stress_prime_count = 0;
volatile int stress_current = 2;
volatile int stress_counter = 0;

extern "C" void prime_task()
{
    while(1)
    {
        int n = stress_current;
        bool prime = true;
        for(int i = 2; i * i <= n; i++)
        {
            if(n % i == 0)
            {
                prime = false;
                break;
            }
        }
        if(prime)
            stress_prime_count = stress_prime_count + 1;
        stress_current = n + 1;
    }
}

extern "C" void counter_task()
{
    while(1)
    {
        stress_counter = stress_counter + 1;
    }
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

    vfs_init();

    vfs_node* root = ramfs_create_root();

    if(root)
    {
        vfs_mount("/", root);

        ramfs_create_dir(root, "mnt");

        vfs_node* bin_dir = ramfs_create_dir(root, "bin");

        if(bin_dir)
        {
            vfs_node* hello_node = ramfs_create_file(bin_dir, "hello.elf");
            if(hello_node)
            {
                hello_node->write(hello_node, 0, hello_elf_size, hello_elf_data);
            }
        }

        ramfs_create_dir(root, "tmp");

        vfs_node* test_elf_node = ramfs_create_file(root, "test.elf");
        if(test_elf_node)
        {
            test_elf_node->write(test_elf_node, 0, test_elf_size, test_elf_data);
        }
    }

    gdt_init();

    tss_init();

    idt_init();

    pic_remap();

    timer_init(100);

    sleep_init();

    user_map_pages();

    int ata_count = ata_init();

    if(ata_count > 0)
    {
        for(int i = 0; i < ata_count; i++)
        {
            const ata_device* d = ata_get_device(i);

            terminal.write("ata");
            char buf[16];
            itoa(i, buf);
            terminal.write(buf);
            terminal.write(": ");
            terminal.write(d->model);
            terminal.write(" (");
            itoa(d->sectors / 2048, buf);
            terminal.write(buf);
            terminal.write("MB)\n");
        }
    }
    else
    {
        terminal.write("ata: no devices\n");
    }

    if(ata_count > 0)
    {
        vfs_node* fat_root = fat32_mount(0, 0);

        if(fat_root)
        {
            terminal.write("fat32: mounted on /mnt\n");
            vfs_mount("/mnt", fat_root);
        }
    }

    int pci_count = pci_init();

    if(pci_count > 0)
    {
        char buf[16];
        itoa(pci_count, buf);
        terminal.write("pci: ");
        terminal.write(buf);
        terminal.write(" devices\n");
    }
    else
    {
        terminal.write("pci: no devices\n");
    }

    int rtl_count = rtl8139_init_all();

    if(rtl_count > 0)
    {
        for(int i = 0; i < rtl_count; i++)
        {
            const rtl8139_dev* d = rtl8139_get_device(i);

            terminal.write("rtl8139: ");
            char buf[16];

            for(int j = 0; j < 6; j++)
            {
                itoa(d->mac[j], buf);

                if(d->mac[j] < 16)
                {
                    terminal.write("0");
                }

                terminal.write(buf);

                if(j < 5)
                    terminal.write(":");
            }

            terminal.write("\n");
        }
    }
    else
    {
        terminal.write("rtl8139: no device\n");
    }

    net_init();

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

    ipc_queue* shell_q = (ipc_queue*)kmalloc(sizeof(ipc_queue));
    if(shell_q)
    {
        ipc_queue_init(shell_q);
        shell_proc->msg_queue = shell_q;
    }

    Process* prime_proc = process_create_kernel(prime_task);
    Process* counter_proc = process_create_kernel(counter_task);

    if(prime_proc)
    {
        ipc_queue* prime_q = (ipc_queue*)kmalloc(sizeof(ipc_queue));
        if(prime_q)
        {
            ipc_queue_init(prime_q);
            prime_proc->msg_queue = prime_q;
        }
    }
    if(counter_proc)
    {
        ipc_queue* counter_q = (ipc_queue*)kmalloc(sizeof(ipc_queue));
        if(counter_q)
        {
            ipc_queue_init(counter_q);
            counter_proc->msg_queue = counter_q;
        }
    }

    if(!prime_proc)
        terminal.write("kernel: failed to create prime task\n");
    if(!counter_proc)
        terminal.write("kernel: failed to create counter task\n");

    terminal.write(
        "[kernel] system ready\n");

    asm volatile("sti");

    process_first_enter(shell_proc);

    while(1)
    {
        asm volatile("hlt");
    }
}