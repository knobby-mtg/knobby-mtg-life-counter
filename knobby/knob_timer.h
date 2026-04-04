#ifndef _KNOB_TIMER_H
#define _KNOB_TIMER_H

#include "knob_types.h"

// ---------- state ----------
extern bool turn_timer_enabled;
extern bool turn_indicator_visible;
extern bool turn_ui_visible;
extern uint32_t turn_elapsed_ms;
extern uint32_t turn_started_ms;
extern int turn_number;

// ---------- functions ----------
void knob_timer_init(void);
void turn_timer_start_fresh(void);
void turn_timer_reset(void);
uint32_t get_turn_elapsed_ms(void);

// event callbacks used in screen builders
void event_tool_timer(lv_event_t *e);
void event_turn_tap(lv_event_t *e);

#endif // _KNOB_TIMER_H
