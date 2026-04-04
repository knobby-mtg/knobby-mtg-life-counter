#ifndef _KNOB_SCR_MENUS_H
#define _KNOB_SCR_MENUS_H

#include "knob_types.h"

// ---------- screens ----------
extern lv_obj_t *screen_quad_menu;
extern lv_obj_t *screen_tools_menu;
extern lv_obj_t *screen_screen_settings_menu;
extern lv_obj_t *screen_settings;
extern lv_obj_t *screen_battery;

// ---------- functions ----------
void build_quad_screen(lv_obj_t **screen, quad_item_t items[4]);
void build_quad_menus(void);
void build_settings_screen(void);
void build_battery_screen(void);

void refresh_settings_ui(void);
void refresh_battery_ui(void);

void open_quad_menu(void);
void open_settings_screen(void);

#endif // _KNOB_SCR_MENUS_H
