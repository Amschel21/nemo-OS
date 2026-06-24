#pragma once

#include "../scheduler/task.hpp"
#include "../memory/vmm.hpp"

struct Process
{
    uint32_t    pid;
    TaskState   state;
    vmm_space_t address_space;
    uint32_t    kernel_stack;
    Task*       main_task;
};

Process* process_create(TaskEntry entry);
void     process_destroy(Process* proc);
int      process_switch(Process* prev, Process* next);
