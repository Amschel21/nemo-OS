#pragma once

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

struct __attribute__((packed)) GDTEntry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
};

struct __attribute__((packed)) GDTPtr
{
    uint16_t limit;
    uint32_t base;
};

void gdt_init();