#pragma once

#include "../memory/vmm.hpp"

#define USER_CODE_ADDR 0x40000000
#define USER_STACK_ADDR 0x40001000
#define USER_STACK_TOP  (USER_STACK_ADDR + 4096)

void user_map_pages();
void user_install_into_space(vmm_space_t space);
void user_enter_ring3();
void user_process_entry();

uint32_t user_code_entry();
uint32_t user_stack_top_value();
uint32_t user_code_phys_addr();
