#ifndef MEASUREMENT_SCHEDULER_H
#define MEASUREMENT_SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_timer.h"

#define MEASUREMENT_SCHEDULER_NAME "measurement_timer"
static esp_timer_handle_t measurement_timer_handle;

void measurement_scheduler_init(uint32_t period_seconds);
bool measurement_scheduler_should_measure(void);
void measurement_scheduler_acknowledge(void);
esp_err_t delete_timer(void);

#endif