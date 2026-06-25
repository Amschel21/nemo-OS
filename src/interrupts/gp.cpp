#include "../kernel/panic.hpp"
#include "../terminal.hpp"
#include "../libk/itoa.hpp"

static char gp_buf[16];

static void gp_print(const char* label, uint32_t v)
{
    itoa(v, gp_buf);
    terminal.write(label);
    terminal.write(gp_buf);
    terminal.write("\n");
}

extern "C" {
    uint32_t gp_err;
    uint32_t gp_eip;
}

extern "C" void gp_handler(uint32_t* stack)
{
    terminal.write("#GP\n");
    gp_print(" err=", gp_err);
    gp_print(" eip=", gp_eip);

    for(int i = 0; i < 12; i++)
    {
        char buf[8] = {0};
        buf[0] = '[';
        buf[1] = '0' + (i / 10);
        buf[2] = '0' + (i % 10);
        buf[3] = ']';
        buf[4] = '=';
        terminal.write(buf);
        gp_print("", stack[i]);
    }

    PANIC("#GP");
}
