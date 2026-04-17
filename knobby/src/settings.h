#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "types.h"

// ---------- screens ----------
extern lv_obj_t *screen_quad_menu;
extern lv_obj_t *screen_tools_menu;
extern lv_obj_t *screen_screen_settings_menu;
extern lv_obj_t *screen_settings_page2;
extern lv_obj_t *screen_settings_page3;
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

#endif // _SETTINGS_H
