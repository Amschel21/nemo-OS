#include "task.hpp"
#include "../memory/kmalloc.hpp"
#include "../libk/memory.hpp"

static Task tasks[16];
static int task_count_value = 0;
static uint32_t next_pid = 1;

extern "C" void task_trampoline();

Task* task_create(TaskEntry entry, Process* process)
{
    uint32_t* stack = (uint32_t*)kmalloc(4096);

    if(!stack)
        return nullptr;

    memset(stack, 0, 4096);

    uint32_t* sp = stack + 1024;

    *--sp = 0x202;
    *--sp = 0x08;
    *--sp = (uint32_t)task_trampoline;

    *--sp = (uint32_t)entry;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;

    asm volatile("cli");

    for(int i = 0; i < 16; i++)
    {
        if(tasks[i].state == TASK_FREE)
        {
            tasks[i].pid = next_pid++;
            tasks[i].state = TASK_READY;
            tasks[i].entry = entry;
            tasks[i].esp = (uint32_t)sp;
            tasks[i].stack_bottom = (uint32_t)stack;
            tasks[i].stack_size = 4096;
            tasks[i].process = process;

            task_count_value++;

            asm volatile("sti");
            return &tasks[i];
        }
    }

    asm volatile("sti");
    kfree(stack);
    return nullptr;
}

Task* task_table()
{
    return tasks;
}

int task_count()
{
    return task_count_value;
}

void task_cleanup_dead()
{
    for(int i = 0; i < 16; i++)
    {
        if(tasks[i].state == TASK_DEAD)
        {
            if(tasks[i].stack_bottom)
                kfree((void*)tasks[i].stack_bottom);

            tasks[i].state = TASK_FREE;
            tasks[i].pid = 0;
            tasks[i].stack_bottom = 0;
            tasks[i].process = nullptr;

            task_count_value--;
        }
    }
}