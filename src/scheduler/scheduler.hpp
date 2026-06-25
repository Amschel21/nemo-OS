#pragma once

#include <stdint.h>

struct Process;

void scheduler_init();
void scheduler_run();

void scheduler_mark_current(int idx);
int  scheduler_current_idx();
Process* scheduler_current_process();

extern "C" uint32_t scheduler_tick(uint32_t current_esp);