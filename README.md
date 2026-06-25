# NemoOS

**NemoOS v1.0** es un sistema operativo para la arquitectura **x86 (i386)** escrito desde cero en **C++20** y **Assembly (GAS)**. Bootea con **GRUB/Multiboot** y se ejecuta en **QEMU**.

## Estado actual

- Shell interactivo con comandos (`help`, `echo`, `ls`, `cat`, `clear`, `ticks`, `mem`, etc.)
- Terminal VGA (80x25, modo texto, color, scroll, cursor) + serial COM1
- Teclado PS/2 (mapa de scancodes US, buffer circular, shift/caps/ctrl/alt)
- Timer PIT (100Hz) con contador de ticks
- GDT (6 entradas: null, kernel code/data, user code/data, TSS)
- IDT (256 entradas: ISR0, IRQ0, IRQ1, #GP, #PF, syscall int 0x80)
- PIC (8259 remapeado a interrupciones 32-47)
- Paginación (identity mapping de toda la RAM, hasta 512MB)
- VMM (Virtual Memory Manager): address spaces, create/switch/map/unmap
- PMM (gestor de memoria física basado en bitmap, alloc/free)
- kmalloc/kfree (heap con free-list, boundary tags, split, coalesce, grow)
- RAMFS (sistema de archivos en RAM, 128 inodos, directorios, archivos)
- Syscalls: `int 0x80` (get_ticks, write terminal, read teclado)
- Scheduler preemptivo (round-robin, context switch via timer IRQ, TSS)
- Procesos kernel con PCB, address space propio, kernel stack
- TSS (Task State Segment para stack ring0 y modo usuario)
- Librería `libk` (`memcpy`, `memset`, `strcmp`, `strcpy`, `itoa`)
- Kernel panic con mensaje, archivo y línea

## Requisitos

- `g++` (x86_64-linux-gnu)
- `ld` (binutils)
- `grub-mkrescue`
- `xorriso`
- `qemu-system-i386`

## Compilar y ejecutar

```bash
make          # Compila y enlaza el kernel
make iso      # Genera imagen ISO booteable
make run      # Ejecuta en QEMU
make debug    # Ejecuta en QEMU con -S -s (espera gdb)
make clean    # Limpia objetos, binarios e ISO
```

## Estructura del proyecto

```
src/
├── boot.s                       # Entry point multiboot
├── kernel.cpp                   # kernel_main: inicialización
├── terminal.cpp/.hpp            # Driver VGA 80x25 + serial COM1
├── arch/x86/
│   ├── ports.cpp/.hpp           # I/O de puertos (inb/outb/inw/outw/inl/outl)
│   └── io.hpp                   # Incluye ports.hpp
├── boot/
│   └── multiboot.cpp/.hpp       # Parsing de información multiboot
├── context/
│   └── context_switch.s/.hpp    # Context switch vía scheduler_tick
├── drivers/
│   ├── keyboard.cpp/.hpp        # Teclado PS/2 (keymap US, shift, caps)
│   ├── keyboard_buffer.cpp/.hpp # Buffer circular 256 bytes
│   └── timer.cpp/.hpp           # Timer PIT 100Hz
├── fs/
│   ├── ramfs.cpp/.hpp           # RAMFS (128 inodos, dirs, archivos)
│   └── vfs.cpp/.hpp             # VFS (stub)
├── interrupts/
│   ├── gdt.cpp/.hpp             # GDT (6 entradas)
│   ├── gdt_flush.s              # lgdt + far jump
│   ├── gp.cpp                   # Handler de #GP
│   ├── idt.cpp/.hpp             # IDT (256 entradas)
│   ├── idt_flush.s              # lidt
│   ├── irq.cpp/.hpp             # Handler IRQ0 (timer ticks, EOI)
│   ├── isr.cpp/.hpp             # default_interrupt_handler
│   ├── isr_stub.s               # Handlers: isr0, irq0, irq1, #GP, #PF
│   ├── keyboard_irq.cpp         # IRQ1: scancode → buffer
│   ├── page_fault.cpp/.hpp      # Page fault handler
│   ├── pic.cpp/.hpp             # PIC 8259: remapeo a ints 32-47
│   ├── syscall_stub.s           # int 0x80 handler
│   ├── tss.cpp/.hpp             # Task State Segment
│   └── tss_flush.s              # ltr
├── ipc/
│   └── sleep.cpp/.hpp           # sleep_ms (basado en timer ticks)
├── kernel/
│   └── panic.cpp/.hpp           # Kernel panic
├── libk/
│   ├── itoa.cpp/.hpp            # itoa
│   ├── memory.cpp/.hpp          # memcpy, memset
│   └── string.cpp/.hpp          # strcmp, strncmp, strcpy
├── memory/
│   ├── heap.cpp/.hpp            # (stub)
│   ├── kmalloc.cpp/.hpp         # Heap: free-list, split, coalesce, grow
│   ├── paging.cpp/.hpp          # Paginación: identity map, map_page
│   ├── pmm.cpp/.hpp             # (stub)
│   ├── pmm_bitmap.cpp/.hpp      # PMM bitmap (alloc/free page)
│   └── vmm.cpp/.hpp             # VMM: address spaces, map/unmap/switch
├── process/
│   └── process.cpp/.hpp         # PCB: crear, destruir, switchear procesos
├── scheduler/
│   ├── scheduler.cpp/.hpp       # Scheduler preemptivo round-robin
│   ├── task.cpp/.hpp            # Tabla de tareas (16 slots)
│   └── task_trampoline.s        # Trampoline para retorno de tareas
├── shell/
│   └── shell.cpp/.hpp           # Shell interactivo
├── syscalls/
│   └── syscall.cpp/.hpp         # Despachador int 0x80
└── user/
    ├── user_entry.s             # Código de usuario Ring 3
    └── user_mode.cpp/.hpp       # Mapeo de páginas de usuario
```

## Licencia

Sin licencia definida.
