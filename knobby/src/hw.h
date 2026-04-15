#ifndef _HW_H
#define _HW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

// ---------- state ----------
extern int brightness_percent;
extern int auto_dim_setting;
extern bool dimmed;
extern float battery_voltage;
extern int battery_percent;

// ---------- tunable power / timing constants (shared) ----------
// Auto-dim timeout is now configurable via NVS (see auto_dim_ms[] in knob_types.h).
// Reducing AUTO_DIM_BRIGHTNESS below 5 further cuts backlight draw while dimmed.
#define AUTO_DIM_BRIGHTNESS     5       /* % brightness while dimmed */
// UNDIM_GRACE_MS suppresses input for this long after wake to avoid accidental presses.
#define UNDIM_GRACE_MS          150     /* ms */
#define CPU_FREQ_ACTIVE         160     /* MHz – APB bus stays 80 MHz at 80/160/240 */
// Battery sample throttle: how often a fresh ADC measurement is allowed.
#define BATTERY_SAMPLE_INTERVAL_MS  60000   /* ms between passive battery measurements */
// Auto-dim check period: how often the inactivity timer fires.
#define AUTO_DIM_CHECK_PERIOD_MS    1000    /* ms */
#define LOW_BATTERY_VOLTAGE         3.35f   /* shutdown threshold (matches 0% curve) */
#define LOW_BATTERY_COUNT           3       /* consecutive low readings before cutoff */
#define LOW_BATTERY_WAKE_US         (15ULL * 1000000ULL)  /* deep sleep wake interval */

// ---------- functions ----------
void knob_hw_init(void);
void brightness_apply(void);
void update_battery_measurement(bool force);
int read_battery_percent(void);
void change_brightness(int delta);
bool in_undim_grace(void);
void knob_enter_deep_sleep(void);

#ifdef __cplusplus
}
#endif

#endif // _HW_H
