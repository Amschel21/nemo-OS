#pragma once

typedef unsigned int uint32_t;

enum TaskState : unsigned char
{
    TASK_FREE    = 0,
    TASK_READY   = 1,
    TASK_RUNNING = 2,
    TASK_BLOCKED = 3,
    TASK_DEAD    = 4
};

typedef void (*TaskEntry)();

struct Process;

struct Task
{
    uint32_t   pid;
    TaskState  state;
    TaskEntry  entry;
    uint32_t   esp;
    uint32_t   stack_bottom;
    uint32_t   stack_size;
    Process*   process;
};

Task* task_create(TaskEntry entry, Process* process = nullptr);
Task* task_table();
int   task_count();

void task_cleanup_dead();