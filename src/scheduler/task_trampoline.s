.text
.global task_trampoline

task_trampoline:
    call *%eax
    call scheduler_mark_dead

1:
    sti
    hlt
    jmp 1b