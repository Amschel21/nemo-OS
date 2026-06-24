#include "multiboot.hpp"

unsigned int total_memory_mb = 0;

static multiboot_info_t* g_mbi = nullptr;

void multiboot_init(multiboot_info_t* mbi)
{
    g_mbi = mbi;

    if(!mbi)
    {
        total_memory_mb = 0;
        return;
    }

    total_memory_mb =
        (mbi->mem_lower +
         mbi->mem_upper) / 1024;
}

multiboot_info_t* multiboot_get_info()
{
    return g_mbi;
}