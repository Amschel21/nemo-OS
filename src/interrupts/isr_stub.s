.global isr0
.global irq0
.global irq1
.global page_fault_stub

.extern irq_handler
.extern keyboard_irq_handler
.extern page_fault_handler
.extern scheduler_tick

isr0:
    cli

isr0_loop:
    hlt
    jmp isr0_loop

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

page_fault_stub:
    pusha

    call page_fault_handler

    popa

    add $4, %esp

    iret