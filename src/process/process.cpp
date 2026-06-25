#include "process.hpp"
#include "../memory/kmalloc.hpp"
#include "../memory/pmm_bitmap.hpp"
#include "../interrupts/tss.hpp"
#include "../kernel/panic.hpp"
#include "../libk/memory.hpp"
#include "../libk/itoa.hpp"
#include "../terminal.hpp"
#include "../user/user_mode.hpp"

#define KERNEL_STACK_BYTES  (2 * 4096)

#define MAX_PROCESSES 16

static uint32_t next_pid = 1;

static Process* process_table[MAX_PROCESSES];
static int process_count = 0;

static void push(uint32_t*& sp, uint32_t v)
{
    *(--sp) = v;
}

static void register_process(Process* proc)
{
    if(process_count >= MAX_PROCESSES)
        PANIC("process: table full");

    process_table[process_count++] = proc;
}

static void* alloc_kernel_stack(uint32_t& out_bottom)
{
    void* lo = pmm_alloc_page();
    void* hi = pmm_alloc_page();

    if(!lo || !hi)
    {
        if(lo) pmm_free_page(lo);
        if(hi) pmm_free_page(hi);
        return nullptr;
    }

    if((uint32_t)hi != (uint32_t)lo + 4096)
    {
        pmm_free_page(lo);
        pmm_free_page(hi);
        return alloc_kernel_stack(out_bottom);
    }

    uint8_t* kstack = (uint8_t*)lo;
    memset(kstack, 0, KERNEL_STACK_BYTES);

    out_bottom = (uint32_t)lo;
    return kstack;
}

static uint32_t* build_user_frame(
    uint32_t* sp,
    uint32_t entry,
    uint32_t user_esp)
{
    //
    // iret ring3 frame
    //
    push(sp, 0x23);      // SS
    push(sp, user_esp);  // ESP
    push(sp, 0x202);     // EFLAGS
    push(sp, 0x1B);      // CS
    push(sp, entry);     // EIP

    //
    // popa frame
    //
    push(sp, 0); // EDI
    push(sp, 0); // ESI
    push(sp, 0); // EBP
    push(sp, 0); // ESP dummy
    push(sp, 0); // EBX
    push(sp, 0); // EDX
    push(sp, 0); // ECX
    push(sp, 0); // EAX

    return sp;
}

static uint32_t* build_kernel_frame(
    uint32_t* sp,
    uint32_t entry,
    uint32_t initial_esp)
{
    (void)initial_esp;

    //
    // iret frame
    //
    push(sp, 0x202); // EFLAGS
    push(sp, 0x08);  // CS
    push(sp, entry); // EIP

    //
    // popa frame
    //
    push(sp, 0); // EDI
    push(sp, 0); // ESI
    push(sp, 0); // EBP
    push(sp, 0); // ESP dummy
    push(sp, 0); // EBX
    push(sp, 0); // EDX
    push(sp, 0); // ECX
    push(sp, 0); // EAX

    return sp;
}

Process* process_create(TaskEntry entry)
{
    Process* proc = (Process*)kmalloc(sizeof(Process));

    if(!proc)
        return nullptr;

    proc->pid = next_pid++;
    proc->state = TASK_READY;
    proc->ring = RING_USER;

    // TEMPORAL:
    // usar espacio kernel para evitar el VMM roto
    proc->address_space = vmm_get_kernel_space();

    proc->kernel_stack = 0;
    proc->kernel_stack_top = 0;
    proc->kernel_stack_bottom = 0;
    proc->saved_esp = 0;

    proc->user_entry = USER_CODE_ADDR;
    proc->user_esp   = USER_STACK_TOP;

    proc->main_task = nullptr;

    uint32_t kstack_bottom = 0;

    uint8_t* kstack =
        (uint8_t*)alloc_kernel_stack(
            kstack_bottom);

    if(!kstack)
    {
        kfree(proc);
        return nullptr;
    }

    uint32_t* sp =
        build_user_frame(
            (uint32_t*)(kstack + KERNEL_STACK_BYTES),
            proc->user_entry,
            proc->user_esp);

    proc->kernel_stack_bottom =
        kstack_bottom;

    proc->kernel_stack_top =
        (uint32_t)sp;

    proc->kernel_stack =
        (uint32_t)kstack +
        KERNEL_STACK_BYTES;

    register_process(proc);

    return proc;
}

Process* process_create_kernel(TaskEntry entry)
{
    Process* proc = (Process*)kmalloc(sizeof(Process));

    if(!proc)
        return nullptr;

    proc->pid = next_pid++;
    proc->state = TASK_READY;
    proc->ring = RING_KERNEL;
    proc->address_space = vmm_get_kernel_space();
    proc->kernel_stack = 0;
    proc->kernel_stack_top = 0;
    proc->kernel_stack_bottom = 0;
    proc->saved_esp = 0;
    proc->user_entry = 0;
    proc->user_esp = 0;
    proc->main_task = nullptr;

    uint32_t kstack_bottom = 0;
    uint8_t* kstack = (uint8_t*)alloc_kernel_stack(kstack_bottom);

    if(!kstack)
    {
        kfree(proc);
        return nullptr;
    }

    uint32_t* sp = build_kernel_frame(
        (uint32_t*)(kstack + KERNEL_STACK_BYTES),
        (uint32_t)entry,
        (uint32_t)kstack + KERNEL_STACK_BYTES);

    proc->kernel_stack_bottom = kstack_bottom;
    proc->kernel_stack_top = (uint32_t)sp;
    proc->kernel_stack = (uint32_t)kstack + KERNEL_STACK_BYTES;

    register_process(proc);
    return proc;
}

void process_destroy(Process* proc)
{
    if(!proc)
        return;

    proc->state = TASK_DEAD;

    if(proc->ring == RING_USER && proc->address_space != vmm_get_kernel_space())
        vmm_destroy_space(proc->address_space);

    if(proc->kernel_stack_bottom)
    {
        pmm_free_page((void*)proc->kernel_stack_bottom);
        pmm_free_page((void*)(proc->kernel_stack_bottom + 4096));
    }

    for(int i = 0; i < process_count; i++)
    {
        if(process_table[i] == proc)
        {
            process_table[i] = process_table[--process_count];
            break;
        }
    }

    kfree(proc);
}

Process* process_get(int index)
{
    if(index < 0 || index >= process_count)
        return nullptr;
    return process_table[index];
}

int process_total()
{
    return process_count;
}

void process_switch(Process* prev, Process* next)
{
    (void)prev;

    if(!next)
        return;

    if(next->kernel_stack)
        tss_set_kernel_stack(next->kernel_stack);

    if(next->address_space)
        vmm_switch(next->address_space);
}

uint32_t process_kernel_esp(Process* proc)
{
    return proc->kernel_stack_top;
}

extern "C" void process_first_enter_asm(uint32_t esp) __attribute__((noreturn));

__asm__(
    ".global process_first_enter_asm\n"
    "process_first_enter_asm:\n"
    "    mov 4(%esp), %esp\n"
    "    popa\n"
    "    iret\n"
);

void process_first_enter(Process* proc)
{
    if(!proc)
        while(1) asm volatile("hlt");

    proc->state = TASK_RUNNING;

    process_switch(nullptr, proc);

    extern void scheduler_mark_current(int idx);
    for(int i = 0; i < 16; i++)
    {
        if(process_get(i) == proc)
        {
            scheduler_mark_current(i);
            break;
        }
    }

    asm volatile("cli");
    process_first_enter_asm(proc->kernel_stack_top);
}

extern "C" void schedule_from_irq0();
