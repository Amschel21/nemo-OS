#pragma once

#include <stdint.h>

void scheduler_init();
void scheduler_run();

void scheduler_mark_current(int idx);

extern "C" uint32_t scheduler_tick(uint32_t current_esp);