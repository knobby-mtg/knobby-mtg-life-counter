#ifndef _KNOB_SCR_MULTIPLAYER_H
#define _KNOB_SCR_MULTIPLAYER_H

#include "knob_types.h"

// ---------- screens ----------
extern lv_obj_t *screen_multiplayer;
extern lv_obj_t *screen_multiplayer_2p;
extern lv_obj_t *screen_multiplayer_3p;
extern lv_obj_t *screen_multiplayer_menu;
extern lv_obj_t *screen_multiplayer_name;
extern lv_obj_t *screen_multiplayer_all_damage;

// ---------- functions ----------
void build_multiplayer_screen(void);
void build_multiplayer_2p_screen(void);
void build_multiplayer_3p_screen(void);
void build_multiplayer_menu_screen(void);
void build_multiplayer_name_screen(void);
void build_multiplayer_all_damage_screen(void);

void refresh_multiplayer_ui(void);
void refresh_multiplayer_menu_ui(void);
void refresh_multiplayer_name_ui(void);
void refresh_multiplayer_all_damage_ui(void);

void open_multiplayer_screen(void);
void open_multiplayer_menu_screen(int player_index);

#endif // _KNOB_SCR_MULTIPLAYER_H
