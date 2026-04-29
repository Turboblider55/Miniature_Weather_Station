#ifndef MEASUREMENT_SCHEDULER_H
#define MEASUREMENT_SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>

void measurement_scheduler_init(uint32_t period_seconds);
bool measurement_scheduler_should_measure(void);
void measurement_scheduler_acknowledge(void);

#endif