#include "shell.hpp"
#include "../terminal.hpp"
#include "../libk/string.hpp"
#include "../drivers/keyboard_buffer.hpp"
#include "../libk/itoa.hpp"
#include "../drivers/timer_ticks.hpp"
#include "../boot/multiboot.hpp"
#include "../memory/pmm_bitmap.hpp"
#include "../memory/kmalloc.hpp"
#include "../fs/ramfs.hpp"

static char cmd[256];
static int idx = 0;

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
    terminal.write(" halt\n");
        terminal.write(" ls\n");
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
    ramfs_ls();
    }
    else if(strncmp(c, "touch ", 6) == 0)
    {
    if(ramfs_create_file(c + 6))
        terminal.write("File created\n");
    else
        terminal.write("Error\n");
    }
    else if (strcmp(c, "halt") == 0)
    {
        terminal.write("CPU halted\n");

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
    else if(strncmp(c, "wc ", 3) == 0)
    {
        char buffer[256];

        if(!ramfs_read(c + 3, buffer))
        {
            terminal.write("File not found\n");
            return;
        }

        int count = 0;

        for(int i = 0; buffer[i]; i++)
            count++;

        char buf[16];
        itoa(count, buf);
        terminal.write(buf);
        terminal.write(" ");
        terminal.write(c + 3);
        terminal.write("\n");
    }
    else if(strncmp(c, "head ", 5) == 0)
    {
        char buffer[256];

        if(!ramfs_read(c + 5, buffer))
        {
            terminal.write("File not found\n");
            return;
        }

        int lines = 0;

        for(int i = 0; buffer[i] && lines < 5; i++)
        {
            terminal.putchar(buffer[i]);

            if(buffer[i] == '\n')
                lines++;
        }

        terminal.write("\n");
    }
    else if(strncmp(c, "rev ", 4) == 0)
    {
        char buffer[256];

        if(!ramfs_read(c + 4, buffer))
        {
            terminal.write("File not found\n");
            return;
        }

        int len = 0;

        for(int i = 0; buffer[i]; i++)
            len++;

        for(int i = len - 1; i >= 0; i--)
            terminal.putchar(buffer[i]);

        terminal.write("\n");
    }
    else if (strncmp(c, "echo ", 5) == 0)
    {
        terminal.write(c + 5);
        terminal.write("\n");
    }
    else if(strncmp(c, "mkdir ", 6) == 0)
    {
    if(ramfs_create_dir(c + 6))
        terminal.write("Directory created\n");
    }
    else if(strncmp(c, "cd ", 3) == 0)
    {
    if(!ramfs_cd(c + 3))
        terminal.write("Directory not found\n");
    }
    else if(strcmp(c, "pwd") == 0)
    {
    terminal.write(ramfs_pwd());
    terminal.write("\n");
    }
    else if(strncmp(c, "cat ", 4) == 0)
{
    char buffer[256];

    if(ramfs_read(c + 4, buffer))
    {
        terminal.write(buffer);
        terminal.write("\n");
    }
    else
    {
        terminal.write("File not found\n");
    }
    }
    else if(strncmp(c, "write ", 6) == 0)
{
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
        terminal.write(
            "usage: write file text\n");

        return;
    }

    *space = 0;

    if(ramfs_write(c + 6, space + 1))
    {
        terminal.write("OK\n");
    }
    else
    {
        terminal.write("File not found\n");
    }
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