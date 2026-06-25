.text
.global user_code_start
.global user_code_end

user_code_start:
    call get_base
get_base:
    pop %ebx
    sub $(get_base - user_code_start), %ebx

    mov $2, %eax
    lea (msg - user_code_start)(%ebx), %ebx
    mov $19, %ecx
    int $0x80

loop:
    mov $1, %eax
    int $0x80
    jmp loop

msg: .ascii "Hello from Ring 3!\n"
user_code_end:
