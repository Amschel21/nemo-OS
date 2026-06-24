# NemoOS

**NemoOS** es un sistema operativo hobby desde cero para la arquitectura **x86 (i386)**. Escrito en **C++20** y **Assembly (GAS)**, bootea con **GRUB/Multiboot** y se ejecuta en **QEMU**.

## Estado actuals

- Shell interactivo con comandos (`help`, `echo`, `ls`, `cat`, `clear`, `ticks`, `mem`, etc.)
- Terminal VGA (80x25, modo texto, color, scroll, cursor)
- Teclado PS/2 (mapa de scancodes US, buffer circular)
- Timer PIT (100Hz) con contador de ticks
- GDT (6 entradas: null, kernel code/data, user code/data, TSS)
- IDT (256 entradas: ISR0, IRQ0, IRQ1, page fault, syscall int 0x80)
- PIC (8259 remapeado a interrupciones 32-47)
- Paginación (mapeo identidad primeros 4MB)
- PMM (gestor de memoria física basado en bitmap)
- Asignador simple (bump allocator en 16MB, sin liberación)
- RAMFS (sistema de archivos en RAM, 128 inodos)
- Syscalls básicas (`int 0x80`, actualmente solo `get_ticks`)
- Planificador cooperativo (16 slots, funciones como tareas)
- TSS (Task State Segment para cambio de modo)
- Librería `libk` (`memcpy`, `memset`, `strcmp`, `strcpy`, `itoa`)
- Pánico del kernel con mensaje, archivo y línea

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
├── boot.s                  # Entry point multiboot
├── kernel.cpp              # Inicialización y main loop
├── terminal.cpp/.hpp       # Driver de terminal VGA
├── arch/x86/
│   ├── ports.cpp/.hpp      # I/O de puertos (inb/outb)
│   └── io.hpp              # Incluye ports.hpp
├── boot/
│   └── multiboot.cpp/.hpp  # Parsing de información multiboot
├── drivers/
│   ├── keyboard.cpp/.hpp   # Driver de teclado
│   ├── keyboard_buffer.cpp/.hpp # Buffer circular
│   └── timer.cpp/.hpp      # Driver de timer PIT
├── interrupts/
│   ├── gdt.cpp/.hpp        # GDT
│   ├── gdt_flush.s         # Carga de GDT
│   ├── idt.cpp/.hpp        # IDT
│   ├── idt_flush.s         # Carga de IDT
│   ├── isr.cpp/.hpp        # Instalación de ISRs
│   ├── isr_stub.s          # Handlers de ISR/IRQ
│   ├── irq.cpp/.hpp        # Manejador de IRQ
│   ├── keyboard_irq.cpp    # IRQ de teclado
│   ├── page_fault.cpp/.hpp # Manejador de page fault
│   ├── pic.cpp/.hpp        # Inicialización de PIC
│   ├── syscall_stub.s      # Handler de int 0x80
│   ├── tss.cpp/.hpp        # TSS
│   └── tss_flush.s         # Carga de TSS
├── kernel/
│   └── panic.cpp/.hpp      # Kernel panic
├── libk/
│   ├── itoa.cpp/.hpp       # itoa
│   ├── memory.cpp/.hpp     # memcpy, memset
│   └── string.cpp/.hpp     # strcmp, strncmp, strcpy
├── memory/
│   ├── heap.cpp/.hpp       # (vacio) Heap
│   ├── kmalloc.cpp/.hpp    # Bump allocator
│   ├── paging.cpp/.hpp     # Paginación
│   ├── pmm.cpp/.hpp        # (vacio) PMM stub
│   ├── pmm_bitmap.cpp/.hpp # PMM basado en bitmap
│   └── vmm.cpp/.hpp        # (vacio) VMM
├── fs/
│   ├── ramfs.cpp/.hpp      # RAMFS
│   └── vfs.cpp/.hpp        # (vacio) VFS
├── scheduler/
│   ├── scheduler.cpp/.hpp  # Planificador
│   └── task.cpp/.hpp       # Tabla de tareas
├── shell/
│   └── shell.cpp/.hpp      # Shell interactivo
└── syscalls/
    └── syscall.cpp/.hpp    # Despachador de syscalls
```

## Licencia

Sin licencia definida. Proyecto personal/hobby.
