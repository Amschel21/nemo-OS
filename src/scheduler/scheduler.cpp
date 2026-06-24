#include "scheduler.hpp"
#include "task.hpp"
#include "../memory/kmalloc.hpp"
#include "../process/process.hpp"

static int current_task = -1;
static uint32_t idle_esp = 0;

extern "C" void scheduler_mark_dead()
{
    if(current_task >= 0)
    {
        Task* tasks = task_table();
        Task* task = &tasks[current_task];
        task->state = TASK_DEAD;

        if(task->process && task->process->main_task == task)
            task->process->state = TASK_DEAD;
    }
}

extern "C" uint32_t scheduler_tick(uint32_t current_esp)
{
    Task* tasks = task_table();
    int prev_idx = current_task;

    if(current_task < 0)
    {
        idle_esp = current_esp;
    }
    else
    {
        if(tasks[current_task].state == TASK_RUNNING)
        {
            tasks[current_task].state = TASK_READY;
            tasks[current_task].esp = current_esp;
        }
    }

    task_cleanup_dead();

    int next = current_task;
    int attempts = 0;

    do {
        next = (next + 1) % 16;
        attempts++;
    } while(attempts < 16 && tasks[next].state != TASK_READY);

    if(attempts >= 16)
    {
        current_task = -1;
        return idle_esp;
    }

    Process* prev_proc = (prev_idx >= 0 && tasks[prev_idx].process)
                            ? tasks[prev_idx].process : nullptr;
    Process* next_proc = tasks[next].process
                            ? tasks[next].process : nullptr;
    process_switch(prev_proc, next_proc);

    tasks[next].state = TASK_RUNNING;
    current_task = next;
    return tasks[next].esp;
}

void scheduler_init()
{
    current_task = -1;
}

void scheduler_run()
{
}