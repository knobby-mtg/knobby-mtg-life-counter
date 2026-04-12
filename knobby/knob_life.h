#ifndef _KNOB_LIFE_H
#define _KNOB_LIFE_H

#include "knob_types.h"

typedef enum {
	COUNTER_TYPE_COMMANDER_TAX = 0,
	COUNTER_TYPE_PARTNER_TAX,
	COUNTER_TYPE_POISON,
	COUNTER_TYPE_EXPERIENCE,
	COUNTER_TYPE_COUNT,
} counter_type_t;

typedef struct {
	const char *menu_label;
	const char *display_name;
	const char *badge_text;
	uint32_t accent_color;
	bool enabled;
} counter_definition_t;

// ---------- state ----------
extern int active_enemy_count;
extern int life_total;
extern int pending_life_delta;
extern bool life_preview_active;
extern enemy_state_t enemies[MAX_ENEMY_COUNT];
extern int selected_enemy;
extern int multiplayer_life[MAX_PLAYERS];
extern int multiplayer_selected;
extern char multiplayer_names[MAX_PLAYERS][16];
extern int multiplayer_menu_player;
extern int multiplayer_cmd_damage_totals[MAX_PLAYERS][MAX_PLAYERS];
extern int cmd_damage_target;
extern int multiplayer_all_damage_value;
extern int multiplayer_pending_life_delta;
extern int multiplayer_preview_player;
extern bool multiplayer_life_preview_active;
extern int dice_result;
extern int multiplayer_counter_values[MAX_PLAYERS][COUNTER_TYPE_COUNT];
extern counter_type_t multiplayer_counter_edit_type;
extern int multiplayer_counter_edit_value;

// ---------- functions ----------
void knob_life_init(void);
void knob_life_reset(void);
void change_life(int delta);
void damage_enter(void);
void add_damage_to_selected_enemy(int delta);
void damage_apply(void);
void damage_cancel(void);
void change_multiplayer_life(int delta);
void change_multiplayer_all_damage(int delta);
void undo_life_change(int player, int delta);
void undo_cmd_damage(int source, int target, int delta);
void undo_counter_change(int player, int counter_type, int delta);
void prepare_cmd_damage_for_player(int target);
void multiplayer_life_preview_commit_cb(lv_timer_t *timer);
void begin_multiplayer_counter_edit(int player, counter_type_t type);
void change_multiplayer_counter_edit(int delta);
int apply_multiplayer_counter_edit(void);
int get_multiplayer_counter_value(int player, counter_type_t type);
const counter_definition_t *get_counter_definition(counter_type_t type);
bool counter_type_is_enabled(counter_type_t type);

// ---------- player colors ----------
lv_color_t get_player_color_vib(int index, int vibrancy);
lv_color_t get_player_base_color(int index);
lv_color_t get_player_active_color(int index);
lv_color_t get_player_text_color(int index);
lv_color_t get_player_preview_color(int index, int delta);
int get_main_player_index(void);
int get_cmd_target_player_index(int row);

#endif // _KNOB_LIFE_H
