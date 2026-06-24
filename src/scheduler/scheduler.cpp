#include "scheduler.hpp"
#include "task.hpp"

extern Task* task_table();

void scheduler_init()
{
}

void scheduler_run()
{
    Task* tasks = task_table();

    for(int i = 0; i < 16; i++)
    {
        if(tasks[i].active)
        {
            tasks[i].entry();
        }
    }
}