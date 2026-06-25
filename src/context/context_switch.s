.global context_switch

.extern scheduler_tick

context_switch:

    pusha

    mov %esp, %eax

    push %eax

    call scheduler_tick

    add $4, %esp

    mov %eax, %esp

    popa

    iret