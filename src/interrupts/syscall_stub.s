.global syscall_stub

.extern syscall_dispatch
.extern scheduler_tick

syscall_stub:

    pusha

    push %edx
    push %ecx
    push %ebx
    push %eax

    call syscall_dispatch

    add $16, %esp

    mov %eax, 28(%esp)

    push %esp
    call scheduler_tick
    add $4, %esp
    mov %eax, %esp

    popa

    iret
