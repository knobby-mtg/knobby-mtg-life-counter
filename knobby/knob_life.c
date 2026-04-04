#include "knob_life.h"
#include "knob_nvs.h"

// Forward declarations for UI refresh (defined in screen modules)
extern void refresh_main_ui(void);
extern void refresh_select_ui(void);
extern void refresh_damage_ui(void);
extern void refresh_multiplayer_ui(void);
extern void refresh_multiplayer_all_damage_ui(void);

// ---------- state ----------
int active_enemy_count = 3;
int life_total = DEFAULT_LIFE_TOTAL;
int pending_life_delta = 0;
bool life_preview_active = false;

enemy_state_t enemies[MAX_ENEMY_COUNT] = {
    {"P1", 0}, {"P2", 0}, {"P3", 0}, {"P4", 0},
    {"P5", 0}, {"P6", 0}, {"P7", 0}
};

int selected_enemy = -1;
int dice_result = 0;

int multiplayer_life[MAX_PLAYERS] = {40, 40, 40, 40, 40, 40, 40, 40};
int multiplayer_selected = 0;
char multiplayer_names[MAX_PLAYERS][16] = {"P1", "P2", "P3", "P4", "P5", "P6", "P7", "P8"};
int multiplayer_menu_player = 0;
int multiplayer_cmd_damage_totals[MAX_PLAYERS][MAX_PLAYERS] = {{0}};
int multiplayer_all_damage_value = 0;
int cmd_damage_target = -1;
static int damage_start_value = 0;
int multiplayer_pending_life_delta = 0;
int multiplayer_preview_player = -1;
bool multiplayer_life_preview_active = false;
static lv_timer_t *life_preview_timer = NULL;
static lv_timer_t *multiplayer_life_preview_timer = NULL;

// ---------- player colors ----------
lv_color_t get_player_base_color(int index)
{
    static const uint32_t colors[MAX_PLAYERS] = {
        0x7B1FE0, 0x29B6F6, 0xFFD600, 0xA5D6A7,
        0xFF6D00, 0xE040FB, 0x00BFA5, 0x8D6E63
    };
    if (index < 0 || index >= MAX_PLAYERS) return lv_color_hex(0x303030);
    return lv_color_hex(colors[index]);
}

lv_color_t get_player_active_color(int index)
{
    static const uint32_t colors[MAX_PLAYERS] = {
        0x9C4DFF, 0x4FC3F7, 0xFFEA61, 0xC8E6C9,
        0xFF9100, 0xEA80FC, 0x64FFDA, 0xBCAAA4
    };
    if (index < 0 || index >= MAX_PLAYERS) return lv_color_hex(0x505050);
    return lv_color_hex(colors[index]);
}

lv_color_t get_player_text_color(int index)
{
    return (index == 2) ? lv_color_black() : lv_color_white();
}

lv_color_t get_player_preview_color(int index, int delta)
{
    if (index == 2) {
        return (delta < 0) ? lv_color_hex(0x7A1020) : lv_color_hex(0x215A2A);
    }
    return (delta < 0) ? lv_palette_main(LV_PALETTE_RED) : lv_palette_main(LV_PALETTE_GREEN);
}

int get_main_player_index(void)
{
    int num = nvs_get_num_players();
    int i;

    for (i = 0; i < num; i++) {
        if (strcmp(multiplayer_names[i], "m") == 0) {
            return i;
        }
    }

    return -1;
}

int get_cmd_target_player_index(int row)
{
    int skip_player;
    int num = nvs_get_num_players();
    int count = 0;
    int i;

    if (row < 0 || row >= active_enemy_count) return row;

    if (cmd_damage_target >= 0) {
        skip_player = cmd_damage_target;
    } else {
        skip_player = get_main_player_index();
    }

    if (skip_player < 0) {
        return row;
    }

    for (i = 0; i < num; i++) {
        if (i == skip_player) continue;
        if (count == row) return i;
        count++;
    }

    return row;
}

// ---------- life preview ----------
static void life_preview_commit_cb(lv_timer_t *timer)
{
    (void)timer;

    life_total = clamp_life(life_total + pending_life_delta);
    pending_life_delta = 0;
    life_preview_active = false;
    multiplayer_pending_life_delta = 0;
    multiplayer_preview_player = -1;
    multiplayer_life_preview_active = false;
    if (life_preview_timer != NULL) {
        lv_timer_pause(life_preview_timer);
    }
    refresh_main_ui();
    refresh_select_ui();
}

void multiplayer_life_preview_commit_cb(lv_timer_t *timer)
{
    (void)timer;

    if (!multiplayer_life_preview_active ||
        multiplayer_preview_player < 0 ||
        multiplayer_preview_player >= nvs_get_players_to_track()) {
        if (multiplayer_life_preview_timer != NULL) {
            lv_timer_pause(multiplayer_life_preview_timer);
        }
        return;
    }

    multiplayer_life[multiplayer_preview_player] = clamp_life(
        multiplayer_life[multiplayer_preview_player] + multiplayer_pending_life_delta
    );
    multiplayer_pending_life_delta = 0;
    multiplayer_preview_player = -1;
    multiplayer_life_preview_active = false;
    if (multiplayer_life_preview_timer != NULL) {
        lv_timer_pause(multiplayer_life_preview_timer);
    }
    refresh_multiplayer_ui();
}

// ---------- life changes ----------
void change_life(int delta)
{
    pending_life_delta += delta;
    pending_life_delta = clamp_life(life_total + pending_life_delta) - life_total;
    life_preview_active = (pending_life_delta != 0);

    if (life_preview_timer != NULL) {
        lv_timer_reset(life_preview_timer);
    }

    if (!life_preview_active && life_preview_timer != NULL) {
        lv_timer_pause(life_preview_timer);
    } else if (life_preview_timer != NULL) {
        lv_timer_resume(life_preview_timer);
    }

    refresh_main_ui();
}

void damage_enter(void)
{
    if (selected_enemy >= 0 && selected_enemy < active_enemy_count)
        damage_start_value = enemies[selected_enemy].damage;
    else
        damage_start_value = 0;
}

void add_damage_to_selected_enemy(int delta)
{
    if (selected_enemy < 0 || selected_enemy >= active_enemy_count) return;

    enemies[selected_enemy].damage += delta;
    if (enemies[selected_enemy].damage < 0)
        enemies[selected_enemy].damage = 0;

    refresh_damage_ui();
}

void damage_apply(void)
{
    int delta;
    int source;

    if (selected_enemy < 0 || selected_enemy >= active_enemy_count) return;

    delta = enemies[selected_enemy].damage - damage_start_value;
    if (delta == 0) return;

    if (cmd_damage_target >= 0) {
        source = get_cmd_target_player_index(selected_enemy);
        multiplayer_cmd_damage_totals[source][cmd_damage_target] = enemies[selected_enemy].damage;
        multiplayer_life[cmd_damage_target] = clamp_life(multiplayer_life[cmd_damage_target] - delta);
    } else {
        change_life(-delta);
    }

    refresh_select_ui();
}

void damage_cancel(void)
{
    if (selected_enemy >= 0 && selected_enemy < active_enemy_count)
        enemies[selected_enemy].damage = damage_start_value;
}

void change_multiplayer_life(int delta)
{
    int preview_base;
    int track = nvs_get_players_to_track();

    if (multiplayer_selected < 0 || multiplayer_selected >= track) return;

    if (multiplayer_life_preview_active &&
        multiplayer_preview_player != multiplayer_selected) {
        multiplayer_life_preview_commit_cb(NULL);
    }

    multiplayer_preview_player = multiplayer_selected;
    preview_base = multiplayer_life[multiplayer_preview_player];
    multiplayer_pending_life_delta += delta;
    multiplayer_pending_life_delta = clamp_life(preview_base + multiplayer_pending_life_delta) - preview_base;
    multiplayer_life_preview_active = (multiplayer_pending_life_delta != 0);

    if (multiplayer_life_preview_timer != NULL) {
        lv_timer_reset(multiplayer_life_preview_timer);
    }

    if (!multiplayer_life_preview_active && multiplayer_life_preview_timer != NULL) {
        lv_timer_pause(multiplayer_life_preview_timer);
        multiplayer_preview_player = -1;
    } else if (multiplayer_life_preview_timer != NULL) {
        lv_timer_resume(multiplayer_life_preview_timer);
    }

    refresh_multiplayer_ui();
}

void prepare_cmd_damage_for_player(int target)
{
    int i, row = 0;
    int num = nvs_get_num_players();

    cmd_damage_target = target;

    for (i = 0; i < num; i++) {
        if (i == target) continue;
        if (row < MAX_ENEMY_COUNT) {
            enemies[row].damage = multiplayer_cmd_damage_totals[i][target];
            row++;
        }
    }
}

void change_multiplayer_all_damage(int delta)
{
    multiplayer_all_damage_value += delta;
    if (multiplayer_all_damage_value < 0) multiplayer_all_damage_value = 0;
    refresh_multiplayer_all_damage_ui();
}

// ---------- reset ----------
void knob_life_reset(void)
{
    int starting_life = nvs_get_life_total();
    int num = nvs_get_num_players();
    int i;

    active_enemy_count = num - 1;
    if (active_enemy_count < 0) active_enemy_count = 0;
    if (active_enemy_count > MAX_ENEMY_COUNT) active_enemy_count = MAX_ENEMY_COUNT;

    life_total = starting_life;
    pending_life_delta = 0;
    life_preview_active = false;
    multiplayer_pending_life_delta = 0;
    multiplayer_preview_player = -1;
    multiplayer_life_preview_active = false;
    selected_enemy = -1;
    dice_result = 0;

    for (i = 0; i < MAX_ENEMY_COUNT; i++) {
        enemies[i].damage = 0;
    }

    for (i = 0; i < MAX_PLAYERS; i++) {
        multiplayer_life[i] = starting_life;
        snprintf(multiplayer_names[i], sizeof(multiplayer_names[i]), "P%d", i + 1);
    }
    multiplayer_selected = 0;
    multiplayer_menu_player = 0;
    cmd_damage_target = -1;
    memset(multiplayer_cmd_damage_totals, 0, sizeof(multiplayer_cmd_damage_totals));
    multiplayer_all_damage_value = 0;

    if (life_preview_timer != NULL) {
        lv_timer_pause(life_preview_timer);
    }
    if (multiplayer_life_preview_timer != NULL) {
        lv_timer_pause(multiplayer_life_preview_timer);
    }
}

// ---------- init ----------
void knob_life_init(void)
{
    int starting_life = nvs_get_life_total();
    int num = nvs_get_num_players();
    int i;

    active_enemy_count = num - 1;
    if (active_enemy_count < 0) active_enemy_count = 0;
    if (active_enemy_count > MAX_ENEMY_COUNT) active_enemy_count = MAX_ENEMY_COUNT;

    life_total = starting_life;
    for (i = 0; i < MAX_PLAYERS; i++) {
        multiplayer_life[i] = starting_life;
    }

    life_preview_timer = lv_timer_create(life_preview_commit_cb, 3000, NULL);
    if (life_preview_timer != NULL) {
        lv_timer_pause(life_preview_timer);
    }
    multiplayer_life_preview_timer = lv_timer_create(multiplayer_life_preview_commit_cb, 3000, NULL);
    if (multiplayer_life_preview_timer != NULL) {
        lv_timer_pause(multiplayer_life_preview_timer);
    }
}
