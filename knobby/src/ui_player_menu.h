#ifndef _UI_PLAYER_MENU_H
#define _UI_PLAYER_MENU_H

#include "types.h"

// ---------- screens ----------
extern lv_obj_t *screen_player_menu;
extern lv_obj_t *screen_player_all_damage;
extern lv_obj_t *screen_counter_menu;
extern lv_obj_t *screen_counter_edit;
extern lv_obj_t *screen_eliminated_player_menu;
extern lv_obj_t *screen_player_color_menu;
extern lv_obj_t *screen_player_color_picker;

// ---------- functions ----------
void build_player_menu_screen(void);
void build_eliminated_player_menu_screen(void);
void build_all_damage_screen(void);
void build_counter_menu_screen(void);
void build_counter_edit_screen(void);
void build_player_color_menu_screen(void);
void build_player_color_picker_screen(void);

void refresh_all_damage_ui(void);
void refresh_counter_edit_ui(void);

void open_player_menu(int player_index);
void open_counter_menu(void);
void change_player_color(int delta);
void commit_player_color(void);

#endif // _UI_PLAYER_MENU_H
