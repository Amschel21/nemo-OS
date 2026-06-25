#pragma once

#include "../scheduler/task.hpp"
#include "../memory/vmm.hpp"
#include "../ipc/ipc.hpp"
#include "../user/user_mode.hpp"

enum ProcessRing : unsigned char
{
    RING_KERNEL = 0,
    RING_USER   = 3
};

struct Process
{
    uint32_t      pid;
    uint32_t      parent_pid;
    TaskState     state;
    ProcessRing   ring;
    vmm_space_t   address_space;
    uint32_t      kernel_stack;
    uint32_t      kernel_stack_top;
    uint32_t      kernel_stack_bottom;
    uint32_t      saved_esp;
    uint32_t      user_entry;
    uint32_t      user_esp;
    int           exit_code;
    Task*         main_task;
    ipc_queue*    msg_queue;
};

Process* process_create(TaskEntry entry, uint32_t user_entry = USER_CODE_ADDR, uint32_t user_esp = USER_STACK_TOP);
Process* process_create_kernel(TaskEntry entry);

void     process_destroy(Process* proc);

void     process_switch(Process* prev, Process* next);
void     process_first_enter(Process* proc) __attribute__((noreturn));

Process* process_get(int index);
int      process_total();
Process* process_current();

void     process_exit_current(int code) __attribute__((noreturn));

uint32_t process_kernel_esp(Process* proc);

void process_set_entry(Process* proc, uint32_t entry, uint32_t user_esp);
