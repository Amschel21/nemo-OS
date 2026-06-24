#pragma once

typedef void (*TaskEntry)();

struct Task
{
    bool active;
    TaskEntry entry;
};

void task_create(TaskEntry entry);