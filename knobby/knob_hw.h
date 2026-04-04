#ifndef _KNOB_HW_H
#define _KNOB_HW_H

#include "knob_types.h"

// ---------- state ----------
extern int brightness_percent;
extern bool auto_dim_enabled;
extern bool dimmed;
extern float battery_voltage;
extern int battery_percent;

// ---------- functions ----------
void knob_hw_init(void);
void brightness_apply(void);
void update_battery_measurement(bool force);
int read_battery_percent(void);
void change_brightness(int delta);
bool in_undim_grace(void);

#endif // _KNOB_HW_H
