#ifndef _MANA_H
#define _MANA_H

#include "types.h"

#define MANA_COLOR_COUNT 6

extern lv_obj_t *screen_mana;
extern int mana_values[MANA_COLOR_COUNT];

void build_mana_screen(void);
void refresh_mana_ui(void);
void open_mana_screen(void);
void change_mana_value(int delta);
void mana_set_selected(int idx);
void mana_discard_preview(void);
void mana_clear_all(void);
void event_tool_mana(lv_event_t *e);

#endif // _MANA_H
