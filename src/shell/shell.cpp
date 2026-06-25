#include "shell.hpp"
#include "../terminal.hpp"
#include "../libk/string.hpp"
#include "../drivers/keyboard_buffer.hpp"
#include "../libk/itoa.hpp"
#include "../drivers/timer_ticks.hpp"
#include "../drivers/ata.hpp"
#include "../drivers/pci.hpp"
#include "../drivers/rtl8139.hpp"
#include "../boot/multiboot.hpp"
#include "../memory/pmm_bitmap.hpp"
#include "../memory/kmalloc.hpp"
#include "../fs/vfs.hpp"
#include "../fs/ramfs.hpp"
#include "../fs/elf.hpp"
#include "../process/process.hpp"
#include "../scheduler/task.hpp"
#include "../ipc/ipc.hpp"
#include "../libk/string.hpp"
#include "../memory/vmm.hpp"
#include "../arch/x86/ports.hpp"

extern volatile int stress_prime_count;
extern volatile int stress_current;
extern volatile int stress_counter;



static char cmd[256];
static int idx = 0;
static vfs_node* cwd = nullptr;

static void* test_page = nullptr;

static void execute(const char* c)
{
    if (strcmp(c, "help") == 0)
    {
        terminal.write("Comandos:\n");
        terminal.write(" help\n");
        terminal.write(" clear\n");
        terminal.write(" echo <texto>\n");
        terminal.write(" about\n");
        terminal.write(" whoami\n");
        terminal.write(" uname\n");
        terminal.write(" uptime\n");
        terminal.write(" yes [texto]\n");
        terminal.write(" true\n");
        terminal.write(" false\n");
        terminal.write(" ticks\n");
        terminal.write(" mem\n");
        terminal.write(" pages\n");
        terminal.write(" alloc\n");
        terminal.write(" free\n");
    terminal.write(" kmalloc\n");
    terminal.write(" testheap\n");
        terminal.write(" ps\n");
        terminal.write(" elfinfo <file>\n");
        terminal.write(" ring3\n");
        terminal.write(" exec <file>\n");
        terminal.write(" send <pid> <msg>\n");
        terminal.write(" recv\n");
        terminal.write(" kill <pid>\n");
        terminal.write(" halt\n");
        terminal.write(" reboot\n");
        terminal.write(" ls\n");
        terminal.write(" pci\n");
        terminal.write(" netstat\n");
        terminal.write(" lsdisk\n");
        terminal.write(" rd <dev> <lba>\n");
        terminal.write(" touch <file>\n");
        terminal.write(" cat <file>\n");
        terminal.write(" wc <file>\n");
        terminal.write(" head <file>\n");
        terminal.write(" rev <file>\n");
        terminal.write(" write <file> <text>\n");
        terminal.write(" mkdir <dir>\n");
        terminal.write(" cd <dir>\n");
        terminal.write(" pwd\n");
    }
    else if (strcmp(c, "clear") == 0)
    {
        terminal.clear();
    }
    else if (strcmp(c, "about") == 0)
    {
        terminal.write("NemoOS v1.0\n");
        terminal.write("Kernel x86 escrito en C++20\n");
    }
    else if (strcmp(c, "ticks") == 0)
    {
        char buf[32];

        itoa(timer_ticks, buf);

        terminal.write(buf);
        terminal.write("\n");
    }
    else if (strcmp(c, "mem") == 0)
    {
        char buf[32];

        itoa(total_memory_mb, buf);

        terminal.write("RAM: ");
        terminal.write(buf);
        terminal.write(" MB\n");
    }
    else if (strcmp(c, "pages") == 0)
    {
        char buf[32];

        terminal.write("Total: ");
        itoa(pmm_total_pages(), buf);
        terminal.write(buf);

        terminal.write("\nUsed : ");
        itoa(pmm_used_pages(), buf);
        terminal.write(buf);

        terminal.write("\nFree : ");
        itoa(pmm_free_pages(), buf);
        terminal.write(buf);

        terminal.write("\n");
    }
    else if (strcmp(c, "alloc") == 0)
    {
        test_page = pmm_alloc_page();

        if (test_page)
            terminal.write("Page allocated\n");
        else
            terminal.write("Out of memory\n");
    }
    else if (strcmp(c, "free") == 0)
    {
        if (test_page)
        {
            pmm_free_page(test_page);
            test_page = nullptr;

            terminal.write("Page freed\n");
        }
        else
        {
            terminal.write("No page allocated\n");
        }
    }
    else if (strcmp(c, "kmalloc") == 0)
    {
        void* ptr = kmalloc(128);

        char buf[32];

        itoa((unsigned int)ptr, buf);

        terminal.write("PTR: ");
        terminal.write(buf);

        if(ptr)
        {
            char* data = (char*)ptr;

            for(int i = 0; i < 10; i++)
                data[i] = 'A' + i;

            data[10] = 0;

            terminal.write(" DATA: ");
            terminal.write(data);
        }

        terminal.write("\n");
    }
    else if(strcmp(c, "testheap") == 0)
    {
        void* a = kmalloc(16);
        void* b = kmalloc(32);
        void* c = kmalloc(64);

        if(!a || !b || !c)
        {
            terminal.write("alloc fail\n");
            return;
        }

        ((char*)a)[0] = 'X';
        ((char*)b)[0] = 'Y';
        ((char*)c)[0] = 'Z';

        kfree(b);
        kfree(a);

        void* d = kmalloc(16);

        terminal.write("reuse: ");
        terminal.write(d == a ? "yes" : "no");
        terminal.write("\n");

        kfree(c);
        kfree(d);

        terminal.write("heap ok\n");
    }
    else if(strcmp(c, "ls") == 0)
    {
        if(!cwd)
        {
            terminal.write("no fs\n");
        }
        else
        {
            char buf[16];
            vfs_node entry;

            for(int i = 0; ; i++)
            {
                if(cwd->readdir(cwd, i, &entry) < 0)
                    break;

                if(entry.flags & FS_DIR)
                {
                    terminal.write("[DIR] ");
                    itoa(i, buf);
                    terminal.write(buf);
                    terminal.write(" ");
                }

                terminal.write(entry.name);
                terminal.write("\n");
            }
        }
    }
    else if(strncmp(c, "touch ", 6) == 0)
    {
        if(!cwd)
        {
            terminal.write("no fs\n");
        }
        else if(ramfs_create_file(cwd, c + 6))
        {
            terminal.write("created\n");
        }
        else
        {
            terminal.write("error\n");
        }
    }
    else if(strcmp(c, "pci") == 0)
    {
        int count = pci_device_count();

        if(count == 0)
        {
            terminal.write("no pci devices\n");
        }
        else
        {
            char buf[16];

            for(int i = 0; i < count; i++)
            {
                const pci_device* d = pci_get_device(i);

                itoa(d->bus, buf);
                terminal.write(buf);
                terminal.write(":");
                itoa(d->slot, buf);
                terminal.write(buf);
                terminal.write(".");
                itoa(d->func, buf);
                terminal.write(buf);
                terminal.write("  vid=0x");
                itoa(d->vendor_id, buf);
                terminal.write(buf);
                terminal.write(" did=0x");
                itoa(d->device_id, buf);
                terminal.write(buf);
                terminal.write(" class=");
                itoa(d->class_code, buf);
                terminal.write(buf);
                terminal.write(" subclass=");
                itoa(d->subclass, buf);
                terminal.write(buf);
                terminal.write("\n");
            }
        }
    }
    else if(strcmp(c, "netstat") == 0)
    {
        int count = rtl8139_count();

        if(count == 0)
        {
            terminal.write("no network devices\n");
        }
        else
        {
            for(int i = 0; i < count; i++)
            {
                const rtl8139_dev* d = rtl8139_get_device(i);
                char buf[16];

                terminal.write("rtl8139#");
                itoa(i, buf);
                terminal.write(buf);
                terminal.write(" MAC ");

                for(int j = 0; j < 6; j++)
                {
                    itoa(d->mac[j], buf);

                    if(d->mac[j] < 16)
                        terminal.write("0");

                    terminal.write(buf);

                    if(j < 5)
                        terminal.write(":");
                }

                terminal.write("\n");
            }
        }
    }
    else if(strcmp(c, "lsdisk") == 0)
    {
        int count = ata_device_count();

        if(count == 0)
        {
            terminal.write("no ata devices\n");
        }
        else
        {
            char buf[16];

            for(int i = 0; i < count; i++)
            {
                const ata_device* d = ata_get_device(i);

                if(!d || !d->present)
                    continue;

                itoa(i, buf);
                terminal.write("ata");
                terminal.write(buf);
                terminal.write(": ");
                terminal.write(d->model);
                terminal.write(" ");
                itoa(d->sectors / 2048, buf);
                terminal.write(buf);
                terminal.write("MB");
                terminal.write(d->is_master ? " master" : " slave");
                terminal.write("\n");
            }
        }
    }
    else if(strncmp(c, "rd ", 3) == 0)
    {
        int dev = 0;
        uint32_t lba = 0;
        const char* s = c + 3;

        while(*s >= '0' && *s <= '9')
        {
            dev = dev * 10 + (*s - '0');
            s++;
        }

        while(*s == ' ') s++;

        while(*s >= '0' && *s <= '9')
        {
            lba = lba * 10 + (*s - '0');
            s++;
        }

        uint8_t buffer[512];
        char buf[16];

        if(ata_read_sector(dev, lba, buffer) == 0)
        {
            terminal.write("sector ");
            itoa(lba, buf);
            terminal.write(buf);
            terminal.write(":\n");

            for(int row = 0; row < 16; row++)
            {
                for(int col = 0; col < 16; col++)
                {
                    uint8_t byte = buffer[row * 16 + col];

                    if(byte >= 32 && byte < 127)
                        terminal.putchar(byte);
                    else
                        terminal.putchar('.');
                }

                terminal.write("  ");

                for(int col = 0; col < 16; col++)
                {
                    uint8_t byte = buffer[row * 16 + col];
                    char hex[4];
                    hex[0] = "0123456789ABCDEF"[(byte >> 4) & 0xF];
                    hex[1] = "0123456789ABCDEF"[byte & 0xF];
                    hex[2] = ' ';
                    hex[3] = 0;
                    terminal.write(hex);
                }

                terminal.write("\n");
            }
        }
        else
        {
            terminal.write("read failed\n");
        }
    }
    else if(strcmp(c, "ps") == 0)
    {
        char buf[32];
        char state_names[][8] = {"FREE", "READY", "RUN", "BLK", "DEAD"};

        for(int i = 0; i < process_total(); i++)
        {
            Process* p = process_get(i);

            if(!p)
                continue;

            itoa(p->pid, buf);
            terminal.write("PID:");
            terminal.write(buf);
            terminal.write(" PPID:");
            itoa(p->parent_pid, buf);
            terminal.write(buf);
            terminal.write(" ");
            terminal.write(state_names[p->state]);
            terminal.write(p->ring == RING_KERNEL ? " KERN" : " USER");
            if(p->state == TASK_DEAD)
            {
                terminal.write(" code:");
                itoa(p->exit_code, buf);
                terminal.write(buf);
            }
            terminal.write("\n");
        }

        terminal.write("stress: prime=");
        itoa(stress_prime_count, buf);
        terminal.write(buf);
        terminal.write(" cur=");
        itoa(stress_current, buf);
        terminal.write(buf);
        terminal.write(" counter=");
        itoa(stress_counter, buf);
        terminal.write(buf);
        terminal.write("\n");
    }
    else if(strncmp(c, "kill ", 5) == 0)
    {
        int pid = 0;
        const char* s = c + 5;

        while(*s >= '0' && *s <= '9')
        {
            pid = pid * 10 + (*s - '0');
            s++;
        }

        bool found = false;

        for(int i = 0; i < process_total(); i++)
        {
            Process* p = process_get(i);

            if(p && (int)p->pid == pid)
            {
                p->state = TASK_DEAD;
                found = true;
                break;
            }
        }

        terminal.write(found ? "killed\n" : "not found\n");
    }
    else if(strncmp(c, "elfinfo ", 8) == 0)
    {
        if(!cwd)
        {
            terminal.write("no fs\n");
            return;
        }

        vfs_node* file = cwd->finddir(cwd, c + 8);

        if(!file || !(file->flags & FS_FILE))
        {
            terminal.write("not found\n");
            return;
        }

        elf_info info;

        if(elf_load(file, &info) == 0)
        {
            char buf[16];

            terminal.write("elf: entry=0x");
            itoa(info.entry, buf);
            terminal.write(buf);
            terminal.write(" load=0x");
            itoa(info.load_addr, buf);
            terminal.write(buf);
            terminal.write(" size=");
            itoa(info.size, buf);
            terminal.write(buf);
            terminal.write("\n");
        }
        else
        {
            terminal.write("elf: load failed\n");
        }
    }
    else if(strncmp(c, "send ", 5) == 0)
    {
        const char* arg = c + 5;
        char pids[16];
        int j = 0;

        while(*arg >= '0' && *arg <= '9' && j < 15)
            pids[j++] = *arg++;
        pids[j] = '\0';

        if(*arg != ' ')
        {
            terminal.write("usage: send <pid> <msg>\n");
            return;
        }

        arg++;
        int pid = 0;
        const char* pp = pids;
        while(*pp) pid = pid * 10 + (*pp++ - '0');

        int ret = ipc_send(pid, arg, strlen(arg) + 1);

        if(ret == 0)
            terminal.write("sent\n");
        else
            terminal.write("send failed\n");
    }
    else if(strcmp(c, "recv") == 0)
    {
        char buf[IPC_MAX_MSG_SIZE];
        uint32_t sender;
        int ret = ipc_recv(&sender, buf, sizeof(buf));

        if(ret > 0)
        {
            char tmp[32];
            terminal.write("from PID ");
            itoa(sender, tmp);
            terminal.write(tmp);
            terminal.write(": ");
            terminal.write(buf);
            terminal.write("\n");
        }
        else
        {
            terminal.write("no messages\n");
        }
    }
    else if(strncmp(c, "exec ", 5) == 0)
    {
        if(!cwd)
        {
            terminal.write("no fs\n");
            return;
        }

        vfs_node* file = cwd->finddir(cwd, c + 5);

        if(!file || !(file->flags & FS_FILE))
        {
            terminal.write("not found\n");
            return;
        }

        elf_info info;
        Process* uproc = process_create(nullptr, 0, 0);

        if(!uproc)
        {
            terminal.write("process creation failed\n");
            return;
        }

        vmm_space_t old_space = vmm_get_kernel_space();

        vmm_switch(uproc->address_space);

        int ret = elf_load_into(file, uproc->address_space, &info);

        vmm_switch(old_space);

        if(ret != 0)
        {
            process_destroy(uproc);
            terminal.write("elf load failed\n");
            return;
        }

        process_set_entry(uproc, info.entry, USER_STACK_TOP);

        char buf[16];
        terminal.write("exec PID=");
        itoa(uproc->pid, buf);
        terminal.write(buf);
        terminal.write(" entry=0x");
        itoa(info.entry, buf);
        terminal.write(buf);
        terminal.write("\n");
    }
    else if(strcmp(c, "ring3") == 0)
    {
        Process* uproc = process_create(nullptr, USER_CODE_ADDR, USER_STACK_TOP);

        if(uproc)
        {
            terminal.write("user process created PID=");
            char buf[16];
            itoa(uproc->pid, buf);
            terminal.write(buf);
            terminal.write("\n");
        }
        else
        {
            terminal.write("user process creation failed\n");
        }
    }
    else if (strcmp(c, "halt") == 0)
    {
        terminal.write("CPU halted\n");

        while(true)
        {
            asm volatile("hlt");
        }
    }
    else if (strcmp(c, "reboot") == 0)
    {
        terminal.write("rebooting...\n");

        uint8_t good;

        do {
            good = inb(0x64);
        } while(good & 0x02);

        outb(0x64, 0xFE);

        while(true)
        {
            asm volatile("hlt");
        }
    }
    else if (strcmp(c, "whoami") == 0)
    {
        terminal.write("nemo\n");
    }
    else if (strcmp(c, "uname") == 0)
    {
        terminal.write("NemoOS\n");
    }
    else if (strcmp(c, "uptime") == 0)
    {
        char buf[32];
        terminal.write("up ");
        itoa(timer_ticks, buf);
        terminal.write(buf);
        terminal.write(" ticks\n");
    }
    else if (strcmp(c, "true") == 0)
    {
    }
    else if (strcmp(c, "false") == 0)
    {
        terminal.write("false\n");
    }
    else if (strncmp(c, "yes", 3) == 0)
    {
        const char* text = c + 3;

        while(*text == ' ') text++;

        if(*text == 0)
            text = "y";

        for(int i = 0; i < 5; i++)
        {
            terminal.write(text);
            terminal.write("\n");
        }
    }
    else if(strncmp(c, "wc ", 3) == 0 || strncmp(c, "head ", 5) == 0 || strncmp(c, "rev ", 4) == 0)
    {
        if(!cwd)
        {
            terminal.write("no fs\n");
            return;
        }

        const char* fname = c + (c[1] == 'c' ? 3 : (c[1] == 'e' ? 5 : 4));
        vfs_node* file = cwd->finddir(cwd, fname);

        if(!file || !(file->flags & FS_FILE))
        {
            terminal.write("not found\n");
            return;
        }

        char buffer[256];
        int got = file->read(file, 0, 255, buffer);
        buffer[got] = 0;

        if(c[1] == 'w')
        {
            char buf[16];
            itoa(got, buf);
            terminal.write(buf);
            terminal.write(" ");
            terminal.write(fname);
            terminal.write("\n");
        }
        else if(c[1] == 'e')
        {
            int lines = 0;
            for(int i = 0; buffer[i] && lines < 5; i++)
            {
                terminal.putchar(buffer[i]);
                if(buffer[i] == '\n')
                    lines++;
            }
            terminal.write("\n");
        }
        else
        {
            int len = 0;
            while(buffer[len]) len++;
            for(int i = len - 1; i >= 0; i--)
                terminal.putchar(buffer[i]);
            terminal.write("\n");
        }
    }
    else if (strncmp(c, "echo ", 5) == 0)
    {
        terminal.write(c + 5);
        terminal.write("\n");
    }
    else if(strncmp(c, "mkdir ", 6) == 0)
    {
        if(!cwd)
        {
            terminal.write("no fs\n");
        }
        else if(ramfs_create_dir(cwd, c + 6))
        {
            terminal.write("created\n");
        }
        else
        {
            terminal.write("error\n");
        }
    }
    else if(strncmp(c, "cd ", 3) == 0)
    {
        if(!cwd)
        {
            terminal.write("no fs\n");
            return;
        }

        vfs_node* dir = vfs_resolve_relative(cwd, c + 3);

        if(!dir || !(dir->flags & FS_DIR))
        {
            terminal.write("not found\n");
        }
        else
        {
            cwd = dir;
        }
    }
    else if(strcmp(c, "pwd") == 0)
    {
        if(!cwd)
        {
            terminal.write("no fs\n");
        }
        else if(cwd->parent == cwd)
        {
            terminal.write("/\n");
        }
        else
        {
            char path[256];
            int pi = 0;
            vfs_node* nodes[64];
            int ni = 0;
            vfs_node* cur = cwd;

            while(cur && cur->parent != cur)
            {
                nodes[ni++] = cur;
                cur = cur->parent;

                if(ni >= 64)
                    break;
            }

            path[pi++] = '/';

            for(int i = ni - 1; i >= 0; i--)
            {
                for(int j = 0; nodes[i]->name[j]; j++)
                    path[pi++] = nodes[i]->name[j];

                if(i > 0)
                    path[pi++] = '/';
            }

            path[pi] = 0;
            terminal.write(path);
            terminal.write("\n");
        }
    }
    else if(strncmp(c, "cat ", 4) == 0)
    {
        if(!cwd)
        {
            terminal.write("no fs\n");
            return;
        }

        vfs_node* file = cwd->finddir(cwd, c + 4);

        if(!file || !(file->flags & FS_FILE))
        {
            terminal.write("not found\n");
            return;
        }

        char buffer[256];
        int got = file->read(file, 0, 255, buffer);
        buffer[got] = 0;
        terminal.write(buffer);
        terminal.write("\n");
    }
    else if(strncmp(c, "write ", 6) == 0)
    {
        if(!cwd)
        {
            terminal.write("no fs\n");
            return;
        }

        char* space = nullptr;

        for(int i = 6; c[i]; i++)
        {
            if(c[i] == ' ')
            {
                space = (char*)&c[i];
                break;
            }
        }

        if(!space)
        {
            terminal.write("usage: write file text\n");
            return;
        }

        *space = 0;

        vfs_node* file = cwd->finddir(cwd, c + 6);

        if(!file || !(file->flags & FS_FILE))
        {
            terminal.write("not found\n");
            return;
        }

        int len = 0;
        while(space[1 + len]) len++;

        if(file->write(file, 0, len, space + 1) > 0)
            terminal.write("OK\n");
        else
            terminal.write("error\n");
    }
    else
    {
        terminal.write("unknown command\n");
    }
}

void shell_on_key(char c)
{
    if (c == '\n')
    {
        terminal.write("\n");

        cmd[idx] = 0;

        execute(cmd);

        idx = 0;

        terminal.write("> ");

        return;
    }

    if (c == '\b')
    {
        if (idx > 0)
        {
            idx--;

            terminal.backspace();
        }

        return;
    }

    if (idx < 255)
    {
        cmd[idx++] = c;

        terminal.putchar(c);
    }
}

void shell_init()
{
    idx = 0;

    cwd = vfs_get_root();

    if(!cwd)
        terminal.write("vfs: no root\n");

    terminal.write("> ");
}

void shell_tick()
{
    char c;

    while (keyboard_buffer_pop(&c))
    {
        shell_on_key(c);
    }
}