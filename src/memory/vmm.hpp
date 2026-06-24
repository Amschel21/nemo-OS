#pragma once

typedef unsigned int uint32_t;

typedef uint32_t vmm_space_t;

#define VMM_PRESENT  0x001
#define VMM_WRITABLE 0x002
#define VMM_USER     0x004

void       vmm_init();
vmm_space_t vmm_get_kernel_space();

vmm_space_t vmm_create_space();
void       vmm_destroy_space(vmm_space_t space);

int vmm_map(
    vmm_space_t space,
    uint32_t virt,
    uint32_t phys,
    uint32_t flags);

int vmm_unmap(
    vmm_space_t space,
    uint32_t virt);

void vmm_switch(vmm_space_t space);
