#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>

void sched_init(void);
void thread_create(void (*entry_point)(void));
uint64_t sched_tick(uint64_t current_rsp);

#define SCHED_SLICE 10  // switch every 10 timer ticks

#endif