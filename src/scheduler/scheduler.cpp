#include "scheduler.hpp"
#include "../process/process.hpp"
#include "../terminal.hpp"
#include "../libk/itoa.hpp"
#include "../interrupts/tss.hpp"

static int current_process = -1;

extern "C" void scheduler_mark_dead()
{
    Process* p = (current_process >= 0)
        ? process_get(current_process) : nullptr;

    if(p) p->state = TASK_DEAD;
}

extern "C" uint32_t scheduler_tick(uint32_t current_esp);

static int find_next(int from)
{
    int total = process_total();
    if(total == 0)
        return -1;

    for(int step = 1; step <= total; step++)
    {
        int idx = (from + step) % total;
        Process* p = process_get(idx);
        if(p && p->state != TASK_DEAD && p->state != TASK_BLOCKED)
            return idx;
    }

    return -1;
}


extern "C" uint32_t scheduler_tick(uint32_t current_esp)
{
    int total = process_total();

    if(total == 0)
        return current_esp;

    int prev = current_process;
    Process* prev_p =
        (prev >= 0)
            ? process_get(prev)
            : nullptr;

    if(prev_p)
    {
        if(prev_p->state == TASK_RUNNING)
        {
            prev_p->state = TASK_READY;
            prev_p->saved_esp = current_esp;
        }
        else if(prev_p->state == TASK_BLOCKED)
        {
            prev_p->saved_esp = current_esp;
        }
    }

    int next_idx = find_next(prev);

    Process* next_p =
        (next_idx >= 0)
            ? process_get(next_idx)
            : nullptr;

    if(!next_p)
        return current_esp;

    process_switch(prev_p, next_p);

    next_p->state = TASK_RUNNING;
    current_process = next_idx;

    if(next_p->saved_esp)
        return next_p->saved_esp;

    return next_p->kernel_stack_top;
}

int scheduler_current_idx()
{
    return current_process;
}

Process* scheduler_current_process()
{
    if(current_process < 0)
        return nullptr;
    return process_get(current_process);
}

void scheduler_init()
{
    current_process = -1;
}

void scheduler_mark_current(int idx)
{
    current_process = idx;
}

void scheduler_run()
{
}
