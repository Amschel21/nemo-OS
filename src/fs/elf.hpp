#pragma once
#include "../fs/vfs.hpp"
#include "../memory/vmm.hpp"

struct elf_info
{
    uint32_t entry;
    uint32_t load_addr;
    uint32_t size;
};

// Load an ELF binary from a VFS node into memory
// Returns 0 on success, -1 on error
int elf_load(vfs_node* node, elf_info* info);

// Load an ELF binary into a specific address space
int elf_load_into(vfs_node* node, vmm_space_t space, elf_info* info);
