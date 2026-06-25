#pragma once

void panic(
    const char* reason,
    const char* file,
    int line);

void kernel_panic(const char* msg);

#define PANIC(msg) panic(msg, __FILE__, __LINE__)