#pragma once

#include <stdint.h>

#define IPC_MAX_MSG_SIZE 64
#define IPC_MAX_MSG_PER_PROC 16

struct ipc_msg
{
    uint32_t sender_pid;
    uint32_t size;
    char data[IPC_MAX_MSG_SIZE];
};

struct ipc_queue
{
    ipc_msg msgs[IPC_MAX_MSG_PER_PROC];
    int head;
    int tail;
    int count;
};

void         ipc_queue_init(ipc_queue* q);
int          ipc_send(uint32_t target_pid, const void* data, uint32_t size);
int          ipc_recv(uint32_t* sender, void* buf, uint32_t size);
