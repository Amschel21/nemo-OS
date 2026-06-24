.text
.global task_trampoline
task_trampoline:
    call *%eax
    call scheduler_mark_dead
    sti
1:
    hlt
    jmp 1b
