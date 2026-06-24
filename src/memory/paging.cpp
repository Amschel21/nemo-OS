#include "paging.hpp"

static uint32_t page_directory[1024]
__attribute__((aligned(4096)));

static uint32_t first_page_table[1024]
__attribute__((aligned(4096)));

void paging_init()
{
    for(uint32_t i = 0; i < 1024; i++)
    {
        first_page_table[i] =
            (i * 0x1000) | 3;
    }

    for(uint32_t i = 0; i < 1024; i++)
    {
        page_directory[i] = 0;
    }

    page_directory[0] =
        ((uint32_t)first_page_table) | 3;

    asm volatile(
        "mov %0, %%cr3"
        :
        : "r"(page_directory)
    );

    uint32_t cr0;

    asm volatile(
        "mov %%cr0, %0"
        : "=r"(cr0)
    );

    cr0 |= 0x80000000;

    asm volatile(
        "mov %0, %%cr0"
        :
        : "r"(cr0)
    );
}