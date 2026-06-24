#include "terminal.hpp"
#include "arch/x86/ports.hpp"

static void serial_init()
{
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
}

static void serial_putchar(char c)
{
    while(!(inb(0x3F8 + 5) & 0x20));
    outb(0x3F8, c);

    if(c == '\n')
    {
        while(!(inb(0x3F8 + 5) & 0x20));
        outb(0x3F8, '\r');
    }
}

static const int WIDTH = 80;
static const int HEIGHT = 25;

static uint16_t* VGA = (uint16_t*)0xB8000;
static uint8_t color = 0x07;

Terminal terminal;

static inline uint16_t entry(char c)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

void Terminal::init()
{
    buffer = VGA;
    row = 0;
    col = 0;
    clear();

    serial_init();
    serial_putchar('\n');
}

void Terminal::clear()
{
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            buffer[y * WIDTH + x] = entry(' ');
        }
    }
    row = 0;
    col = 0;
    update_cursor();
}

void Terminal::scroll()
{
    for (int y = 1; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            buffer[(y - 1) * WIDTH + x] =
                buffer[y * WIDTH + x];
        }
    }

    for (int x = 0; x < WIDTH; x++) {
        buffer[(HEIGHT - 1) * WIDTH + x] = entry(' ');
    }

    row = HEIGHT - 1;
}

void Terminal::update_cursor()
{
    int pos = row * WIDTH + col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void Terminal::putchar(char c)
{
    serial_putchar(c);

    if (c == '\n') {
        col = 0;
        row++;
    }
    else if (c == '\b') {
        if (col > 0) col--;
        buffer[row * WIDTH + col] = entry(' ');
    }
    else {
        buffer[row * WIDTH + col] = entry(c);
        col++;
    }

    if (col >= WIDTH) {
        col = 0;
        row++;
    }

    if (row >= HEIGHT) {
        scroll();
    }

    update_cursor();
}

void Terminal::write(const char* str)
{
    for (int i = 0; str[i]; i++) {
        putchar(str[i]);
    }
}

void Terminal::backspace()
{
    putchar('\b');
}