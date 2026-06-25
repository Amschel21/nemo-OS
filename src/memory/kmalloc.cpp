#include "kmalloc.hpp"
#include "pmm_bitmap.hpp"
#include "../kernel/panic.hpp"
#include "../libk/memory.hpp"

#define HEAP_MAGIC 0xDEAD
#define MIN_BLOCK  32
#define HEAP_GROW  (64 * 1024)

struct heap_header
{
    uint32_t    size;
    uint32_t    magic;
    heap_header* next;
    heap_header* prev;
};

#define HEADER_SIZE 16

static heap_header* free_list = nullptr;
static void* heap_end = nullptr;

static inline bool is_valid(heap_header* h)
{
    return h->magic == HEAP_MAGIC;
}

static inline heap_header* header_from_ptr(void* ptr)
{
    return (heap_header*)((uint8_t*)ptr - HEADER_SIZE);
}

static inline void* ptr_from_header(heap_header* h)
{
    return (void*)((uint8_t*)h + HEADER_SIZE);
}

static inline void set_footer(heap_header* h)
{
    *(uint32_t*)((uint8_t*)h + h->size - 4) = h->size;
}

static inline heap_header* next_block(heap_header* h)
{
    return (heap_header*)((uint8_t*)h + h->size);
}

static inline heap_header* prev_block(heap_header* h)
{
    uint32_t prev_size = *(uint32_t*)((uint8_t*)h - 4);
    return (heap_header*)((uint8_t*)h - prev_size);
}

static inline bool has_next(heap_header* h)
{
    return (void*)((uint8_t*)h + h->size) < heap_end;
}

static inline bool has_prev(heap_header* h)
{
    return (uint8_t*)h > (uint8_t*)heap_end - HEAP_GROW;
}

static void list_remove(heap_header* h)
{
    if(h->prev)
        h->prev->next = h->next;

    if(h->next)
        h->next->prev = h->prev;

    if(free_list == h)
        free_list = h->next;
}

static void list_append(heap_header* h)
{
    h->next = nullptr;
    h->prev = nullptr;

    if(!free_list)
    {
        free_list = h;
        return;
    }

    heap_header* last = free_list;

    while(last->next)
        last = last->next;

    last->next = h;
    h->prev = last;
}

void kmalloc_init()
{
    heap_header* initial =
        (heap_header*)pmm_alloc_page();

    if(!initial)
        PANIC("kmalloc_init: OOM");

    heap_end = (uint8_t*)initial + 4096;

    initial->size = 4096;
    initial->magic = HEAP_MAGIC;

    set_footer(initial);

    free_list = initial;
    initial->next = nullptr;
    initial->prev = nullptr;
}

static void heap_grow()
{
    uint32_t pages = (HEAP_GROW + 4095) / 4096;
    void* first = pmm_alloc_page();

    if(!first)
        return;

    bool contiguous = true;

    for(uint32_t i = 1; i < pages; i++)
    {
        void* page = pmm_alloc_page();

        if(!page)
        {
            contiguous = false;
            break;
        }

        if((uint32_t)page != (uint32_t)first + i * 4096)
        {
            pmm_free_page(page);
            contiguous = false;
            break;
        }
    }

    if(!contiguous)
    {
        for(uint32_t i = 0; i < pages; i++)
        {
            void* page = pmm_alloc_page();

            if(!page)
                break;

            heap_header* block = (heap_header*)page;

            block->size = 4096;
            block->magic = HEAP_MAGIC;

            heap_end = (uint8_t*)page + 4096;

            set_footer(block);

            list_append(block);
        }

        return;
    }

    heap_header* block = (heap_header*)first;

    block->size = pages * 4096;
    block->magic = HEAP_MAGIC;

    heap_end = (uint8_t*)first + block->size;

    set_footer(block);

    list_append(block);
}

void* kmalloc(size_t size)
{
    if(size == 0)
        return nullptr;

    size = (size + 7) & ~7;

    uint32_t needed = size + HEADER_SIZE + 4;

    if(needed < MIN_BLOCK)
        needed = MIN_BLOCK;

    heap_header* h = free_list;

    while(h)
    {
        if(h->size >= needed)
        {
            uint32_t leftover = h->size - needed;

            if(leftover >= MIN_BLOCK)
            {
                heap_header* split =
                    (heap_header*)(
                        (uint8_t*)h + needed);

                split->size = leftover;
                split->magic = HEAP_MAGIC;

                set_footer(split);

                split->next = h->next;
                split->prev = h->prev;

                if(h->next)
                    h->next->prev = split;

                if(h->prev)
                    h->prev->next = split;
                else
                    free_list = split;

                h->size = needed;
            }
            else
            {
                list_remove(h);
            }

            set_footer(h);

            memset(ptr_from_header(h), 0,
                   h->size - HEADER_SIZE - 4);

            return ptr_from_header(h);
        }

        h = h->next;
    }

    heap_grow();

    h = free_list;

    while(h)
    {
        if(h->size >= needed)
        {
            uint32_t leftover = h->size - needed;

            if(leftover >= MIN_BLOCK)
            {
                heap_header* split =
                    (heap_header*)(
                        (uint8_t*)h + needed);

                split->size = leftover;
                split->magic = HEAP_MAGIC;

                set_footer(split);

                split->next = h->next;
                split->prev = h->prev;

                if(h->next)
                    h->next->prev = split;

                if(h->prev)
                    h->prev->next = split;
                else
                    free_list = split;

                h->size = needed;
            }
            else
            {
                list_remove(h);
            }

            set_footer(h);

            memset(ptr_from_header(h), 0,
                   h->size - HEADER_SIZE - 4);

            return ptr_from_header(h);
        }

        h = h->next;
    }

    return nullptr;
}

void kfree(void* ptr)
{
    if(!ptr)
        return;

    heap_header* h = header_from_ptr(ptr);

    if(!is_valid(h))
        return;

    bool merge_prev = false;
    bool merge_next = false;

    if(has_prev(h))
    {
        heap_header* p = prev_block(h);

        if(p->magic == HEAP_MAGIC)
            merge_prev = true;
    }

    if(has_next(h))
    {
        heap_header* n = next_block(h);

        if(n->magic == HEAP_MAGIC)
            merge_next = true;
    }

    if(merge_prev && merge_next)
    {
        heap_header* p = prev_block(h);
        heap_header* n = next_block(h);

        list_remove(p);
        list_remove(n);

        p->size += h->size + n->size;

        set_footer(p);

        list_append(p);
    }
    else if(merge_prev)
    {
        heap_header* p = prev_block(h);

        list_remove(p);

        p->size += h->size;

        set_footer(p);
    }
    else if(merge_next)
    {
        heap_header* n = next_block(h);

        list_remove(n);

        h->size += n->size;

        set_footer(h);

        list_append(h);
    }
    else
    {
        h->magic = HEAP_MAGIC;

        set_footer(h);

        list_append(h);
    }
}