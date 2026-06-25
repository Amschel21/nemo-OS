#pragma once

typedef unsigned int uint32_t;

extern "C" void isr0();
extern "C" void irq0();
extern "C" void irq1();
extern "C" void default_isr();

extern "C" void default_interrupt_handler();

void isr_install();