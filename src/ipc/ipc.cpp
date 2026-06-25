#include "ipc.hpp"
#include "../process/process.hpp"
#include "../scheduler/scheduler.hpp"
#include "../libk/memory.hpp"

void ipc_queue_init(ipc_queue* q)
{
    q->head = 0;
    q->tail = 0;
    q->count = 0;
}

int ipc_send(uint32_t target_pid, const void* data, uint32_t size)
{
    if(!data || size == 0 || size > IPC_MAX_MSG_SIZE)
        return -1;

    for(int i = 0; i < process_total(); i++)
    {
        Process* p = process_get(i);
        if(!p || p->pid != target_pid)
            continue;

        if(p->state == TASK_DEAD)
            return -1;

        ipc_queue* q = p->msg_queue;
        if(!q)
            return -1;

        if(q->count >= IPC_MAX_MSG_PER_PROC)
            return -1;

        ipc_msg* msg = &q->msgs[q->tail];
        msg->sender_pid = scheduler_current_process()
            ? scheduler_current_process()->pid : 0;
        msg->size = size;
        memcpy(msg->data, data, size);

        q->tail = (q->tail + 1) % IPC_MAX_MSG_PER_PROC;
        q->count++;

        if(p->state == TASK_BLOCKED)
            p->state = TASK_READY;

        return 0;
    }

    return -1;
}

int ipc_recv(uint32_t* sender, void* buf, uint32_t size)
{
    Process* me = scheduler_current_process();
    if(!me || !buf || size == 0)
        return -1;

    ipc_queue* q = me->msg_queue;
    if(!q)
        return -1;

    while(q->count == 0)
    {
        me->state = TASK_BLOCKED;
        return 0;
    }

    ipc_msg* msg = &q->msgs[q->head];
    uint32_t copy_size = msg->size < size ? msg->size : size;

    if(sender)
        *sender = msg->sender_pid;

    memcpy(buf, msg->data, copy_size);

    q->head = (q->head + 1) % IPC_MAX_MSG_PER_PROC;
    q->count--;

    return (int)copy_size;
}
