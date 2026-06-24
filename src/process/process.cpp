#include "process.hpp"
#include "../memory/kmalloc.hpp"

static uint32_t next_pid = 1;

Process* process_create(TaskEntry entry)
{
    Process* proc = (Process*)kmalloc(sizeof(Process));

    if(!proc)
        return nullptr;

    proc->pid = next_pid++;
    proc->state = TASK_READY;
    proc->address_space = vmm_create_space();
    proc->kernel_stack = 0;
    proc->main_task = nullptr;

    if(!proc->address_space)
    {
        kfree(proc);
        return nullptr;
    }

    Task* task = task_create(entry, proc);

    if(!task)
    {
        vmm_destroy_space(proc->address_space);
        kfree(proc);
        return nullptr;
    }

    proc->main_task = task;
    proc->kernel_stack = task->stack_bottom + task->stack_size;
    return proc;
}

void process_destroy(Process* proc)
{
    if(!proc)
        return;

    proc->state = TASK_DEAD;

    if(proc->main_task)
        proc->main_task->state = TASK_DEAD;

    vmm_destroy_space(proc->address_space);
    kfree(proc);
}

int process_switch(Process* prev, Process* next)
{
    if(!next)
        return 0;

    if(!next->address_space)
        return 0;

    if(prev == next)
        return 0;

    vmm_switch(next->address_space);
    return 0;
}
