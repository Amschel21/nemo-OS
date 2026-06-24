#include "pmm_bitmap.hpp"

static constexpr uint32_t PAGE_SIZE = 4096;
static constexpr uint32_t MAX_RAM_MB = 1024;

static constexpr uint32_t MAX_PAGES =
    (MAX_RAM_MB * 1024 * 1024) / PAGE_SIZE;

static uint8_t bitmap[MAX_PAGES / 8];

static uint32_t total_pages = 0;
static uint32_t used_pages = 0;

static inline void set_bit(uint32_t bit)
{
    bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void clear_bit(uint32_t bit)
{
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline bool test_bit(uint32_t bit)
{
    return bitmap[bit / 8] & (1 << (bit % 8));
}

void pmm_init(uint32_t memory_mb)
{
    total_pages =
        (memory_mb * 1024 * 1024) / PAGE_SIZE;

    static constexpr uint32_t RESERVED_PAGES = 1024;

    used_pages = RESERVED_PAGES;

    if(used_pages > total_pages)
        used_pages = total_pages;

    for(uint32_t i = 0; i < sizeof(bitmap); i++)
        bitmap[i] = 0;

    for(uint32_t i = 0; i < used_pages; i++)
        set_bit(i);
}

void* pmm_alloc_page()
{
    for(uint32_t i = 0; i < total_pages; i++)
    {
        if(!test_bit(i))
        {
            set_bit(i);
            used_pages++;

            return (void*)(i * PAGE_SIZE);
        }
    }

    return nullptr;
}

void pmm_free_page(void* page)
{
    uint32_t index =
        ((uint32_t)page) / PAGE_SIZE;

    if(index >= total_pages)
        return;

    if(test_bit(index))
    {
        clear_bit(index);
        used_pages--;
    }
}

uint32_t pmm_free_pages()
{
    return total_pages - used_pages;
}

uint32_t pmm_used_pages()
{
    return used_pages;
}

uint32_t pmm_total_pages()
{
    return total_pages;
}