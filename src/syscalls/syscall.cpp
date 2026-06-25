#include "syscall.hpp"

#include "../drivers/timer.hpp"
#include "../terminal.hpp"
#include "../drivers/keyboard_buffer.hpp"
#include "../process/process.hpp"
#include "../scheduler/scheduler.hpp"
#include "../ipc/ipc.hpp"

extern "C"
unsigned int syscall_dispatch(
    unsigned int eax,
    unsigned int ebx,
    unsigned int ecx,
    unsigned int edx)
{
    (void)edx;

    switch(eax)
    {
        case 1:
            return timer_ticks;

        case 2:
        {
            const char* str = (const char*)ebx;

            if(!str)
                return 0;

            unsigned int len = ecx;

            for(unsigned int i = 0; i < len; i++)
            {
                if(!str[i])
                    break;

                terminal.putchar(str[i]);
            }

            return len;
        }

        case 3:
        {
            char* buf = (char*)ebx;

            if(!buf || ecx == 0)
                return 0;

            unsigned int max = ecx;
            unsigned int count = 0;

            while(count < max)
            {
                char c;

                if(keyboard_buffer_pop(&c))
                {
                    buf[count++] = c;
                }
                else
                {
                    break;
                }
            }

            return count;
        }

        case 4:
        {
            Process* p = scheduler_current_process();
            if(p)
            {
                p->exit_code = (int)ebx;
                p->state = TASK_DEAD;
            }
            return 0;
        }

        case 5:
        {
            Process* me = scheduler_current_process();
            if(!me)
                return 0xFFFFFFFF;

            int pid = (int)ebx;

            for(int i = 0; i < process_total(); i++)
            {
                Process* child = process_get(i);

                if(!child || child->parent_pid != me->pid)
                    continue;

                if(pid >= 0 && (int)child->pid != pid)
                    continue;

                if(child->state == TASK_DEAD)
                {
                    int exit_code = child->exit_code;
                    uint32_t child_pid = child->pid;

                    process_destroy(child);
                    (void)exit_code;

                    return child_pid;
                }
            }

            return 0xFFFFFFFF;
        }

        case 6:
        {
            int pid = (int)ebx;

            for(int i = 0; i < process_total(); i++)
            {
                Process* p = process_get(i);

                if(p && (int)p->pid == pid)
                {
                    p->state = TASK_DEAD;
                    return 0;
                }
            }

            return 0xFFFFFFFF;
        }

        case 7:
        {
            int pid = (int)ebx;
            return (unsigned int)ipc_send((uint32_t)pid, (const void*)ecx, edx);
        }

        case 8:
        {
            void* buf = (void*)ebx;
            uint32_t size = ecx;
            return (unsigned int)ipc_recv(nullptr, buf, size);
        }

        default:
            return 0xFFFFFFFF;
    }
}