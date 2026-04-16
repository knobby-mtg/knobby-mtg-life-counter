#ifndef _GAME_H
#define _GAME_H

#include "types.h"

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
	const char *icon_text;
	uint32_t accent_color;
	bool enabled;
} counter_definition_t;

// ---------- state ----------
extern int active_enemy_count;
extern enemy_state_t enemies[MAX_ENEMY_COUNT];
extern int selected_enemy;
extern int player_life[MAX_DISPLAY_PLAYERS];
extern int selected_player;
extern char player_names[MAX_GAME_PLAYERS][16];
extern int menu_player;
extern int cmd_damage_totals[MAX_GAME_PLAYERS][MAX_DISPLAY_PLAYERS];
extern int cmd_damage_target;
extern int all_damage_value;
extern int pending_life_delta;
extern int preview_player;
extern bool life_preview_active;
extern int dice_result;
extern int player_counters[MAX_DISPLAY_PLAYERS][COUNTER_TYPE_COUNT];
extern counter_type_t counter_edit_type;
extern int counter_edit_value;
extern bool player_eliminated[MAX_DISPLAY_PLAYERS];

// ---------- functions ----------
void knob_life_init(void);
void knob_life_reset(void);
void damage_enter(void);
void add_damage_to_selected_enemy(int delta);
void damage_apply(void);
void damage_cancel(void);
void change_player_life(int delta);
void change_all_damage(int delta);
void undo_life_change(int player, int delta);
void undo_cmd_damage(int source, int target, int delta);
void undo_counter_change(int player, int counter_type, int delta);
void prepare_cmd_damage_for_player(int target);
void life_preview_commit_cb(lv_timer_t *timer);
void begin_counter_edit(int player, counter_type_t type);
void change_counter_edit(int delta);
int apply_counter_edit(void);
int get_counter_value(int player, counter_type_t type);
const counter_definition_t *get_counter_definition(counter_type_t type);
bool counter_type_is_enabled(counter_type_t type);

bool elimination_action_available(int player);
void undo_elimination_action(int player);
void manual_eliminate_player(int player);
void manual_uneliminate_player(int player);

void check_player_elimination(int player);

// ---------- player colors ----------
extern int player_color_index[MAX_DISPLAY_PLAYERS];
extern bool player_life_color[MAX_DISPLAY_PLAYERS];
extern bool player_has_override[MAX_DISPLAY_PLAYERS];

lv_color_t get_player_color_vib(int index, int vibrancy);
lv_color_t get_player_base_color(int index);
lv_color_t get_player_active_color(int index);
lv_color_t get_player_text_color(int index);
lv_color_t get_player_preview_color(int index, int delta);
lv_color_t get_custom_color_vib(int index, int vibrancy);
const char *get_custom_color_name(int index);
lv_color_t get_effective_player_color(int player_i, int color_i, int vibrancy);
int get_main_player_index(void);
int get_cmd_target_player_index(int row);

#endif // _GAME_H
