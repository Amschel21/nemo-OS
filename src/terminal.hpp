#pragma once
#include <stdint.h>

class Terminal {
public:
    void init();
    void clear();

    void putchar(char c);
    void write(const char* str);
    void backspace();

private:
    void scroll();
    void update_cursor();

    uint16_t* buffer;

    int row;
    int col;
};

extern Terminal terminal;