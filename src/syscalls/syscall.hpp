#pragma once

extern "C" unsigned int syscall_dispatch(
    unsigned int eax,
    unsigned int ebx,
    unsigned int ecx,
    unsigned int edx);