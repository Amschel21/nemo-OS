#include "elf.hpp"
#include "../memory/kmalloc.hpp"
#include "../memory/pmm_bitmap.hpp"
#include "../memory/vmm.hpp"
#include "../libk/memory.hpp"

#define ELF_MAGIC 0x464C457F

#define PT_NULL   0
#define PT_LOAD   1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE   4
#define PT_PHDR   6

#define PF_X 1
#define PF_W 2
#define PF_R 4

#pragma pack(push, 1)

struct elf32_header
{
    uint32_t magic;
    uint8_t  cls;
    uint8_t  data;
    uint8_t  version;
    uint8_t  osabi;
    uint8_t  abiversion;
    uint8_t  pad[7];
    uint16_t type;
    uint16_t machine;
    uint32_t elf_version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
};

struct elf32_phdr
{
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
};

#pragma pack(pop)

int elf_load_into(vfs_node* node, vmm_space_t space, elf_info* info)
{
    if(!node || !info)
        return -1;

    uint32_t total_size = node->size;

    uint8_t* file_buf = (uint8_t*)kmalloc(total_size);

    if(!file_buf)
        return -1;

    if(node->read(node, 0, total_size, file_buf) < 0)
    {
        kfree(file_buf);
        return -1;
    }

    elf32_header* hdr = (elf32_header*)file_buf;

    if(hdr->magic != ELF_MAGIC ||
       hdr->cls != 1 ||
       hdr->data != 1 ||
       hdr->machine != 3 ||
       hdr->type != 2)
    {
        kfree(file_buf);
        return -1;
    }

    info->entry = hdr->entry;
    info->load_addr = 0xFFFFFFFF;
    info->size = 0;

    for(uint16_t i = 0; i < hdr->phnum; i++)
    {
        elf32_phdr* ph = (elf32_phdr*)(file_buf + hdr->phoff +
                                        i * hdr->phentsize);

        if(ph->type != PT_LOAD)
            continue;

        uint32_t start = ph->vaddr & ~0xFFF;
        uint32_t end = (ph->vaddr + ph->memsz + 0xFFF) & ~0xFFF;

        for(uint32_t page = start; page < end; page += 0x1000)
        {
            void* phys = pmm_alloc_page();

            if(!phys)
            {
                kfree(file_buf);
                return -1;
            }

            uint32_t flags = VMM_PRESENT | VMM_USER;

            if(ph->flags & PF_W)
                flags |= VMM_WRITABLE;

            vmm_map(space, page, (uint32_t)phys, flags);
        }

        uint32_t copy_start = ph->vaddr;
        uint32_t copy_size = ph->filesz;

        for(uint32_t j = 0; j < copy_size; j++)
            ((uint8_t*)copy_start)[j] = file_buf[ph->offset + j];

        uint32_t zero_start = ph->vaddr + ph->filesz;
        uint32_t zero_size = ph->memsz - ph->filesz;

        for(uint32_t j = 0; j < zero_size; j++)
            ((uint8_t*)zero_start)[j] = 0;

        if(ph->vaddr < info->load_addr)
            info->load_addr = ph->vaddr;

        if(ph->vaddr + ph->memsz > info->size)
            info->size = ph->vaddr + ph->memsz - info->load_addr;
    }

    kfree(file_buf);
    return 0;
}

int elf_load(vfs_node* node, elf_info* info)
{
    return elf_load_into(node, vmm_get_kernel_space(), info);
}
