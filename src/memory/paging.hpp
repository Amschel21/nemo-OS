#pragma once

typedef unsigned int uint32_t;

void paging_init();
void paging_map_page(uint32_t virt, uint32_t phys, uint32_t flags);
void paging_map_identity(uint32_t phys_start, uint32_t phys_end, uint32_t flags);