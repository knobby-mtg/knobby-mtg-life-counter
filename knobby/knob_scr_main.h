#ifndef _KNOB_SCR_MAIN_H
#define _KNOB_SCR_MAIN_H

#include "knob_types.h"

// ---------- screens ----------
extern lv_obj_t *screen_main;
extern lv_obj_t *screen_select;
extern lv_obj_t *screen_damage;

// ---------- functions ----------
void build_main_screen(void);
void build_select_screen(void);
void build_damage_screen(void);

void refresh_main_ui(void);
void refresh_select_ui(void);
void refresh_damage_ui(void);

void open_select_screen(void);
void back_to_main(void);

#endif // _KNOB_SCR_MAIN_H
