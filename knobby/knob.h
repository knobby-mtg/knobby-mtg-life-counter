#ifndef _KNOB_H
#define _KNOB_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"
#include "bidi_switch_knob.h"

void knob_gui(void);
void knob_cb(lv_event_t *e);

void knob_change(knob_event_t k,int cont);
void knob_process_pending(void);
bool activity_kick(void);
bool knob_is_dimmed(void);
void knob_notify_swipe_up(void);
void knob_notify_swipe_down(void);
float knob_read_battery_voltage(void);


#ifdef __cplusplus
}
#endif

#endif
