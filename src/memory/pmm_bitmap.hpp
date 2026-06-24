#pragma once

typedef unsigned char uint8_t;
typedef unsigned int  uint32_t;

void pmm_init(uint32_t memory_mb);

void* pmm_alloc_page();
void  pmm_free_page(void* page);

uint32_t pmm_free_pages();
uint32_t pmm_used_pages();
uint32_t pmm_total_pages();