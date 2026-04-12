#ifndef _KNOB_SCR_MULTIPLAYER_H
#define _KNOB_SCR_MULTIPLAYER_H

#include "knob_types.h"

// ---------- screens ----------
extern lv_obj_t *screen_4p;
extern lv_obj_t *screen_2p;
extern lv_obj_t *screen_3p;
extern lv_obj_t *screen_player_menu;
extern lv_obj_t *screen_player_all_damage;
extern lv_obj_t *screen_player_counters_menu;
extern lv_obj_t *screen_player_counter_edit;

// ---------- functions ----------
void build_multiplayer_screen(void);
void build_multiplayer_2p_screen(void);
void build_multiplayer_3p_screen(void);
void build_multiplayer_menu_screen(void);
void build_multiplayer_all_damage_screen(void);
void build_multiplayer_counter_menu_screen(void);
void build_multiplayer_counter_edit_screen(void);

void refresh_multiplayer_ui(void);
void refresh_multiplayer_all_damage_ui(void);
void refresh_multiplayer_counter_edit_ui(void);

void open_multiplayer_screen(void);
void open_multiplayer_menu_screen(int player_index);
void open_multiplayer_counter_menu_screen(void);
void select_kick_timer(void);

#endif // _KNOB_SCR_MULTIPLAYER_H
