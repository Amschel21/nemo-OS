#include "panic.hpp"
#include "../terminal.hpp"

void panic(
    const char* reason,
    const char* file,
    int line)
{
    terminal.clear();

    terminal.write("\n");
    terminal.write("=================================\n");
    terminal.write("          KERNEL PANIC\n");
    terminal.write("=================================\n");

    terminal.write("Reason: ");
    terminal.write(reason);
    terminal.write("\n");

    terminal.write("File: ");
    terminal.write(file);
    terminal.write("\n");

    terminal.write("Line: ");

    char num[16];
    int pos = 0;

    int n = line;

    if(n == 0)
    {
        num[pos++] = '0';
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
            num[pos++] = tmp[--t];
        }
    }

    num[pos] = 0;

    terminal.write(num);
    terminal.write("\n");

    asm volatile("cli");

    while(true)
    {
        asm volatile("hlt");
    }
}

void kernel_panic(const char* msg)
{
    panic(
        msg,
        "unknown",
        0);
}