#ifndef _SIM_STUBS_H
#define _SIM_STUBS_H

#include <stdint.h>

/* Controllable tick for LVGL — advance with sim_tick_advance() */
uint32_t sim_millis(void);
void sim_tick_advance(uint32_t ms);

/* Pre-populate the in-memory NVS store before knob_nvs_init() runs.
 * This lets CLI flags control settings that knob_nvs.c reads at init. */
void sim_nvs_preset_i8(const char *key, int8_t value);
void sim_nvs_preset_i16(const char *key, int16_t value);

/* Controllable battery voltage for screenshots */
extern float sim_battery_voltage;

/* ESP32 attribute macros — empty on desktop */
#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

#endif /* _SIM_STUBS_H */
