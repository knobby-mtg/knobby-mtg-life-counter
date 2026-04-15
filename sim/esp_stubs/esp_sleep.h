#ifndef _SIM_ESP_SLEEP_H
#define _SIM_ESP_SLEEP_H

#include "esp_err.h"
#include <stdint.h>

static inline esp_err_t esp_sleep_enable_timer_wakeup(uint64_t time_in_us)
{ (void)time_in_us; return ESP_OK; }

static inline void esp_deep_sleep_start(void) { }

#endif /* _SIM_ESP_SLEEP_H */
