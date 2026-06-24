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
    uint32_t* new_pd = (uint32_t*)pmm_alloc_page();

    if(!new_pd)
        return 0;

    uint32_t* current_pd = (uint32_t*)kernel_space;

    for(int i = 0; i < 1024; i++)
        new_pd[i] = current_pd[i];

    return (vmm_space_t)(uint32_t)new_pd;
}

void vmm_destroy_space(vmm_space_t space)
{
    (void)space;
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

        pd[pd_idx] = ((uint32_t)pt) | VMM_PRESENT | VMM_WRITABLE;
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

void vmm_switch(vmm_space_t space)
{
    asm volatile(
        "mov %0, %%cr3"
        :
        : "r"(space));
}
