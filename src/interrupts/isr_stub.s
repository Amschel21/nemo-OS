.global isr0
.global irq0
.global irq1
.global page_fault_stub
.global default_isr
.global default_vector
.global gp_stub

.extern irq_handler
.extern keyboard_irq_handler
.extern page_fault_handler
.extern scheduler_tick
.extern default_interrupt_handler
.extern gp_handler

default_vector: .long 0

isr0:
    cli

isr0_loop:
    hlt
    jmp isr0_loop

default_isr:
    pusha
    mov %esp, %eax
    add $40, %eax
    mov (%eax), %eax
    mov %eax, default_vector
    call default_interrupt_handler
    popa
    iret

irq0:
    pusha

    call irq_handler

    push %esp
    call scheduler_tick
    add $4, %esp

    mov %eax, %esp

    popa

    iret

irq1:
    pusha

    call keyboard_irq_handler

    popa

    iret

gp_stub:
    pusha
    lea 32(%esp), %eax
    mov (%eax), %eax
    mov %eax, gp_err
    lea 36(%esp), %eax
    mov (%eax), %eax
    mov %eax, gp_eip
    push %esp
    call gp_handler
    add $4, %esp
    popa
    add $4, %esp
    iret

page_fault_stub:
    pusha
    lea 32(%esp), %eax
    mov (%eax), %eax
    mov %eax, pf_err
    call page_fault_handler
    popa
    add $4, %esp
    iret