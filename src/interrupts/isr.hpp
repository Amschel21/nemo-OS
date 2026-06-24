#pragma once

extern "C" void isr0();

extern "C" void irq0();
extern "C" void irq1();

void isr_install();