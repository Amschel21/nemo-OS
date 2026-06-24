.global syscall_stub

.extern syscall_dispatch

syscall_stub:

    pusha

    push %edx
    push %ecx
    push %ebx
    push %eax

    call syscall_dispatch

    add $16, %esp

    mov %eax, 28(%esp)

    popa

    iret