#ifndef _KNOB_DICE_H
#define _KNOB_DICE_H

#include "knob_types.h"

// ---------- state ----------
extern lv_obj_t *screen_dice;

// ---------- functions ----------
void build_dice_screen(void);
void refresh_dice_ui(void);
void open_dice_screen(void);

// event callback used in menu builder
void event_tool_dice(lv_event_t *e);

#endif // _KNOB_DICE_H
