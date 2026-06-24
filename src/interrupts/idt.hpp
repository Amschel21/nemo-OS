#pragma once

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

struct __attribute__((packed)) IDTEntry
{
    uint16_t base_low;
    uint16_t selector;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
};

struct __attribute__((packed)) IDTPtr
{
    uint16_t limit;
    uint32_t base;
};

void idt_init();