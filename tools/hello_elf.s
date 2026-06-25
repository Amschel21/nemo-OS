.section .text
.global _start
_start:
    mov $2, %eax
    mov $msg, %ebx
    mov $15, %ecx
    int $0x80
loop:
    hlt
    jmp loop

msg: .ascii "Hello from ELF!\n"
