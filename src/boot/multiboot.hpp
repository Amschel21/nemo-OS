#pragma once

typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

struct multiboot_info_t
{
    uint32_t flags;

    uint32_t mem_lower;
    uint32_t mem_upper;

    uint32_t boot_device;
    uint32_t cmdline;

    uint32_t mods_count;
    uint32_t mods_addr;

    uint32_t syms[4];

    uint32_t mmap_length;
    uint32_t mmap_addr;
};

struct multiboot_mmap_entry_t
{
    uint32_t size;

    uint64_t addr;
    uint64_t len;

    uint32_t type;
} __attribute__((packed));

extern unsigned int total_memory_mb;

void multiboot_init(multiboot_info_t* mbi);

multiboot_info_t* multiboot_get_info();