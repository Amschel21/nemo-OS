#include "task.hpp"

static Task tasks[16];

void task_create(TaskEntry entry)
{
    for(int i = 0; i < 16; i++)
    {
        if(!tasks[i].active)
        {
            tasks[i].active = true;
            tasks[i].entry = entry;
            return;
        }
    }
}

Task* task_table()
{
    return tasks;
}