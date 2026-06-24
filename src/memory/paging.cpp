#include "paging.hpp"
#include "pmm_bitmap.hpp"
#include "../boot/multiboot.hpp"
#include "../kernel/panic.hpp"
#include "../libk/memory.hpp"

static uint32_t page_directory[1024]
__attribute__((aligned(4096)));

static uint32_t boot_page_table[1024]
__attribute__((aligned(4096)));

void paging_map_page(
    uint32_t virt,
    uint32_t phys,
    uint32_t flags)
{
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    if(!(page_directory[pd_idx] & 1))
    {
        uint32_t* pt = (uint32_t*)pmm_alloc_page();

        if(!pt)
            return;

        memset(pt, 0, 4096);

        page_directory[pd_idx] =
            ((uint32_t)pt) | (flags & 0xFFF) | 1;
    }

    uint32_t* pt = (uint32_t*)(
        page_directory[pd_idx] & 0xFFFFF000);

    pt[pt_idx] = (phys & 0xFFFFF000) | (flags & 0xFFF);

    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

void paging_map_identity(
    uint32_t phys_start,
    uint32_t phys_end,
    uint32_t flags)
{
    phys_start &= ~0xFFF;
    phys_end = (phys_end + 0xFFF) & ~0xFFF;

    for(uint32_t addr = phys_start;
        addr < phys_end;
        addr += 0x1000)
    {
        paging_map_page(addr, addr, flags);
    }
}

void paging_init()
{
    uint32_t total_mb = total_memory_mb;

    if(total_mb == 0)
        total_mb = 32;

    for(uint32_t i = 0; i < 1024; i++)
    {
        boot_page_table[i] =
            (i * 0x1000) | 3;
    }

    for(uint32_t i = 0; i < 1024; i++)
    {
        page_directory[i] = 0;
    }

    page_directory[0] =
        ((uint32_t)boot_page_table) | 3;

    uint32_t end_addr =
        total_mb * 1024 * 1024;

    for(uint32_t addr = 0x400000;
        addr < end_addr;
        addr += 0x1000)
    {
        uint32_t pd_idx = addr >> 22;
        uint32_t pt_idx = (addr >> 12) & 0x3FF;

        if(!(page_directory[pd_idx] & 1))
        {
            uint32_t* pt = (uint32_t*)pmm_alloc_page();

            if(!pt)
                PANIC("paging_init: OOM");

            memset(pt, 0, 4096);

            page_directory[pd_idx] =
                ((uint32_t)pt) | 3;
        }

        uint32_t* pt = (uint32_t*)(
            page_directory[pd_idx] & 0xFFFFF000);

        pt[pt_idx] = addr | 3;
    }

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