#ifndef _KNOB_DAMAGE_LOG_H
#define _KNOB_DAMAGE_LOG_H

#include "knob_types.h"

#define DAMAGE_LOG_MAX 256

typedef enum {
    LOG_EVT_LIFE = 0,
    LOG_EVT_CMD_DAMAGE,
    LOG_EVT_POISON,
    LOG_EVT_COUNTER,
} log_event_type_t;

extern lv_obj_t *screen_damage_log;

void damage_log_add(int player, int delta, uint8_t event_type, int source);
void damage_log_reset(void);
void damage_log_select_next(void);
void damage_log_select_prev(void);
void damage_log_undo_selected(void);

void build_damage_log_screen(void);
void open_damage_log_screen(void);

#endif // _KNOB_DAMAGE_LOG_H
