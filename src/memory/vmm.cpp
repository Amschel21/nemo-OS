#include "vmm.hpp"
#include "pmm_bitmap.hpp"

static vmm_space_t kernel_space = 0;

void vmm_init()
{
    asm volatile(
        "mov %%cr3, %0"
        : "=r"(kernel_space));
}

vmm_space_t vmm_get_kernel_space()
{
    return kernel_space;
}

vmm_space_t vmm_create_space()
{
    uint32_t* new_pd =
        (uint32_t*)pmm_alloc_page();

    if(!new_pd)
        return 0;

    uint32_t* current_pd =
        (uint32_t*)kernel_space;

    for(int i = 0; i < 1024; i++)
    {
        uint32_t entry = current_pd[i];

        if(!(entry & VMM_PRESENT))
        {
            new_pd[i] = 0;
            continue;
        }

        if(entry & VMM_USER)
        {
            uint32_t* new_pt = (uint32_t*)pmm_alloc_page();

            if(!new_pt)
            {
                for(int j = 0; j < i; j++)
                {
                    if(new_pd[j] & VMM_USER)
                        pmm_free_page((void*)(new_pd[j] & 0xFFFFF000));
                }

                pmm_free_page(new_pd);
                return 0;
            }

            uint32_t* old_pt = (uint32_t*)(entry & 0xFFFFF000);

            for(int j = 0; j < 1024; j++)
                new_pt[j] = old_pt[j];

            new_pd[i] = ((uint32_t)new_pt) | (entry & 0xFFF);
        }
        else
        {
            new_pd[i] = entry;
        }
    }

    return (vmm_space_t)(uint32_t)new_pd;
}

void vmm_switch(vmm_space_t space)
{
    asm volatile(
        "mov %0, %%cr3"
        :
        : "r"(space));
}

void vmm_destroy_space(vmm_space_t space)
{
    if(!space)
        return;

    uint32_t* pd = (uint32_t*)space;

    for(int i = 0; i < 1024; i++)
    {
        uint32_t entry = pd[i];

        if(!(entry & VMM_PRESENT))
            continue;

        if(entry & VMM_USER)
            pmm_free_page((void*)(entry & 0xFFFFF000));
    }

    pmm_free_page((void*)space);
}

int vmm_map(
    vmm_space_t space,
    uint32_t virt,
    uint32_t phys,
    uint32_t flags)
{
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    uint32_t* pd = (uint32_t*)space;

    if(!(pd[pd_idx] & VMM_PRESENT))
    {
        uint32_t* pt = (uint32_t*)pmm_alloc_page();

        if(!pt)
            return -1;

        for(int i = 0; i < 1024; i++)
            pt[i] = 0;

        pd[pd_idx] = ((uint32_t)pt) | VMM_PRESENT | VMM_WRITABLE | VMM_USER;
    }

    uint32_t* pt = (uint32_t*)(pd[pd_idx] & 0xFFFFF000);

    pt[pt_idx] = (phys & 0xFFFFF000) | (flags & 0xFFF);

    uint32_t current_cr3;

    asm volatile("mov %%cr3, %0" : "=r"(current_cr3));

    if(current_cr3 == space)
        asm volatile("invlpg (%0)" : : "r"(virt) : "memory");

    return 0;
}

int vmm_unmap(
    vmm_space_t space,
    uint32_t virt)
{
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    uint32_t* pd = (uint32_t*)space;

    if(!(pd[pd_idx] & VMM_PRESENT))
        return 0;

    uint32_t* pt = (uint32_t*)(pd[pd_idx] & 0xFFFFF000);

    pt[pt_idx] = 0;

    uint32_t current_cr3;

    asm volatile("mov %%cr3, %0" : "=r"(current_cr3));

    if(current_cr3 == space)
        asm volatile("invlpg (%0)" : : "r"(virt) : "memory");

    return 0;
}
