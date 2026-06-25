#include "user_mode.hpp"
#include "../terminal.hpp"
#include "../memory/pmm_bitmap.hpp"
#include "../memory/paging.hpp"
#include "../memory/vmm.hpp"
#include "../interrupts/tss.hpp"
#include "../libk/memory.hpp"

extern "C" void user_code_start();
extern "C" void user_code_end();

#define USER_CODE_ADDR 0x40000000
#define USER_STACK_ADDR 0x40001000
#define USER_STACK_TOP  (USER_STACK_ADDR + 4096)

static uint32_t user_code_phys = 0;
static uint32_t user_code_size = 0;

void user_map_pages()
{
    user_code_size =
        (uint32_t)user_code_end -
        (uint32_t)user_code_start;

    if(user_code_size > 4096)
    {
        terminal.write("user: code too big\n");
        return;
    }

    void* page = pmm_alloc_page();
    if(!page)
    {
        terminal.write("user: alloc code page failed\n");
        return;
    }

    user_code_phys = (uint32_t)page;

    memcpy(
        (void*)user_code_phys,
        (void*)user_code_start,
        user_code_size);

    terminal.write("user: code copied\n");
}

void user_install_into_space(vmm_space_t space)
{
    if(!space || !user_code_phys)
        return;

    vmm_map(
        space,
        USER_CODE_ADDR,
        user_code_phys,
        VMM_PRESENT | VMM_USER);

    uint32_t stack_phys = (uint32_t)pmm_alloc_page();

    if(!stack_phys)
        return;

    vmm_map(
        space,
        USER_STACK_ADDR,
        stack_phys,
        VMM_PRESENT | VMM_WRITABLE | VMM_USER);
}

void user_enter_ring3()
{
    terminal.write("user: not used\n");
    while(1) asm volatile("hlt");
}

void user_process_entry()
{
    while(1) asm volatile("hlt");
}

uint32_t user_code_entry()      { return USER_CODE_ADDR; }
uint32_t user_stack_top_value() { return USER_STACK_TOP; }
uint32_t user_code_phys_addr()  { return user_code_phys; }
