#include "game.h"
#include "storage.h"
#include "damage_log.h"
#include "esp_random.h"
// Forward declarations for UI refresh (defined in screen modules)
extern void refresh_player_ui(void);
extern void refresh_select_ui(void);
extern void refresh_damage_ui(void);
extern void refresh_all_damage_ui(void);
extern void select_kick_timer(void);

// ---------- state ----------
int active_enemy_count = 3;

enemy_state_t enemies[MAX_ENEMY_COUNT] = {
    {"P1", 0}, {"P2", 0}, {"P3", 0}, {"P4", 0},
    {"P5", 0}, {"P6", 0}, {"P7", 0}
};

int selected_enemy = -1;
int dice_result = 0;

int player_life[MAX_DISPLAY_PLAYERS] = {40, 40, 40, 40};
int selected_player = -1;
char player_names[MAX_GAME_PLAYERS][16] = {
    "P1", "P2", "P3", "P4", "P5", "P6", "P7", "P8"
};
int menu_player = 0;
int cmd_damage_totals[MAX_GAME_PLAYERS][MAX_DISPLAY_PLAYERS] = {{0}};
int all_damage_value = 0;
int cmd_damage_target = -1;
static int damage_start_value = 0;
int pending_life_delta = 0;
int preview_player = -1;
bool life_preview_active = false;
int player_counters[MAX_DISPLAY_PLAYERS][COUNTER_TYPE_COUNT] = {{0}};
counter_type_t counter_edit_type = COUNTER_TYPE_COMMANDER_TAX;
int counter_edit_value = 0;
bool player_eliminated[MAX_DISPLAY_PLAYERS] = {false};

typedef struct {
    bool valid;
    uint8_t event_type;
    int source;
    int delta;
} elimination_action_t;

static elimination_action_t elimination_action[MAX_DISPLAY_PLAYERS] = {{0}};



static lv_timer_t *life_preview_timer = NULL;

#define MANA_ICON_COMMANDER "\xEE\xA7\x86"
#define MANA_ICON_PARTY     "\xEE\xA6\x87"
#define MANA_ICON_SKULL     "\xEE\x98\x98"
#define MANA_ICON_LEVEL     "\xEE\xA4\x80"

static const counter_definition_t counter_definitions[COUNTER_TYPE_COUNT] = {
    {"Commander\nTax", "Commander Tax", "C", MANA_ICON_COMMANDER, 0xA84300, true},
    {"Partner\nTax", "Partner Tax", "P", MANA_ICON_PARTY, 0x1565C0, true},
    {"Poison", "Poison", "!", MANA_ICON_SKULL, 0x2E7D32, true},
    {"Experience", "Experience", "E", MANA_ICON_LEVEL, 0x6A1B9A, true},
};

static void clear_player_elimination_action(int player)
{
    if (player < 0 || player >= MAX_DISPLAY_PLAYERS) return;
    elimination_action[player].valid = false;
}

static void set_player_elimination_action(int player, uint8_t event_type, int source, int delta)
{
    if (player < 0 || player >= MAX_DISPLAY_PLAYERS) return;
    elimination_action[player].valid = true;
    elimination_action[player].event_type = event_type;
    elimination_action[player].source = source;
    elimination_action[player].delta = delta;
}

bool elimination_action_available(int player)
{
    if (player < 0 || player >= MAX_DISPLAY_PLAYERS) return false;
    return elimination_action[player].valid;
}

void undo_elimination_action(int player)
{
    if (!elimination_action_available(player)) return;

    elimination_action_t action = elimination_action[player];
    clear_player_elimination_action(player);

    if (action.event_type == LOG_EVT_LIFE) {
        undo_life_change(player, action.delta);
    } else if (action.event_type == LOG_EVT_CMD_DAMAGE) {
        undo_life_change(player, action.delta);
        undo_cmd_damage(action.source, player, action.delta);
    } else if (action.event_type == LOG_EVT_COUNTER) {
        undo_counter_change(player, action.source, action.delta);
    }
}

void check_player_elimination(int player)
{
    if (player < 0 || player >= MAX_DISPLAY_PLAYERS) return;
    bool was_eliminated = player_eliminated[player];
    bool now_eliminated = false;

    if (nvs_get_auto_eliminate()) {
        if (player_life[player] <= 0) {
            now_eliminated = true;
        } else {
            for (int i = 0; i < MAX_GAME_PLAYERS; i++) {
                if (i != player && cmd_damage_totals[i][player] >= 20) {
                    now_eliminated = true;
                    break;
                }
            }
            if (!now_eliminated && player_counters[player][COUNTER_TYPE_POISON] >= 10) {
                now_eliminated = true;
            }
        }
    }

    player_eliminated[player] = now_eliminated;
    if (!now_eliminated) {
        clear_player_elimination_action(player);
    }

    if (was_eliminated != now_eliminated) {
        refresh_player_ui();
    }
}

void manual_eliminate_player(int player)
{
    if (player < 0 || player >= MAX_DISPLAY_PLAYERS) return;
    if (player_eliminated[player]) return;
    player_eliminated[player] = true;
    clear_player_elimination_action(player);
    refresh_player_ui();
}

void manual_uneliminate_player(int player)
{
    if (player < 0 || player >= MAX_DISPLAY_PLAYERS) return;
    if (!player_eliminated[player]) return;
    player_eliminated[player] = false;
    clear_player_elimination_action(player);
    refresh_player_ui();
}

// ---------- player colors ----------
static const uint32_t player_color_table[MAX_GAME_PLAYERS][LIFE_VIB_COUNT] = {
    /*  dim        mid        vivid  */
    {0x024D3A, 0x06D6A0, 0x66FFD9},  /* P1 green  (bottom-left) */
    {0x2A0A4D, 0x7B1FE0, 0x9C4DFF},  /* P2 purple (top-left)    */
    {0x0A3A4D, 0x29B6F6, 0x4FC3F7},  /* P3 blue   (top-right)   */
    {0x4D4400, 0xFFD600, 0xFFEA61},  /* P4 yellow (bottom-right) */
    {0x4D1C1C, 0xF44336, 0xFF5252},  /* P5 red    */
    {0x4D3300, 0xFF9800, 0xFFB74D},  /* P6 orange */
    {0x004D4D, 0x00BCD4, 0x4DD0E1},  /* P7 cyan   */
    {0x3D0A4D, 0xE040FB, 0xEA80FC},  /* P8 pink   */
};

// ---------- custom color palette (18 colors) ----------
static const uint32_t custom_color_table[CUSTOM_COLOR_COUNT][LIFE_VIB_COUNT] = {
    /*  dim        mid        vivid  */
    {0x024D3A, 0x06D6A0, 0x66FFD9},  /*  0 Green (logo) */
    {0x2A0A4D, 0x7B1FE0, 0x9C4DFF},  /*  1 Purple */
    {0x0A3A4D, 0x29B6F6, 0x4FC3F7},  /*  2 Blue   */
    {0x4D4400, 0xFFD600, 0xFFEA61},  /*  3 Yellow */
    {0x4D1C1C, 0xF44336, 0xFF5252},  /*  4 Red    */
    {0x4D3300, 0xFF9800, 0xFFB74D},  /*  5 Orange */
    {0x004D4D, 0x00BCD4, 0x4DD0E1},  /*  6 Cyan   */
    {0x3D0A4D, 0xE040FB, 0xEA80FC},  /*  7 Pink   */
    {0x2D4D00, 0x8BC34A, 0xAED581},  /*  8 Lime   */
    {0x0A0A4D, 0x3F51B5, 0x7986CB},  /*  9 Indigo */
    {0x4D0A2A, 0xE91E63, 0xF06292},  /* 10 Rose   */
    {0x333333, 0xAAAAAA, 0xFFFFFF},  /* 11 White  */
    {0x00332E, 0x009688, 0x4DB6AC},  /* 12 Teal   */
    {0x4D3800, 0xFFC107, 0xFFD54F},  /* 13 Amber  */
    {0x2E1F16, 0x795548, 0xA1887F},  /* 14 Brown  */
    {0x1E2D33, 0x607D8B, 0x90A4AE},  /* 15 Gray   */
    {0x1A3D1A, 0xA5D6A7, 0x4CAF50},  /* 16 Sage   */
    {0x0A0A0A, 0x303030, 0x505050},  /* 17 Black  */
};

static const char *custom_color_names[CUSTOM_COLOR_COUNT] = {
    "Green", "Purple", "Blue", "Yellow",
    "Red", "Orange", "Cyan", "Pink",
    "Lime", "Indigo", "Rose", "White",
    "Teal", "Amber", "Brown", "Gray",
    "Sage", "Black",
};

// ---------- per-player color state (runtime only, lost on reboot) ----------
int player_color_index[MAX_DISPLAY_PLAYERS] = {0, 1, 2, 3};
bool player_life_color[MAX_DISPLAY_PLAYERS] = {false, false, false, false};
bool player_has_override[MAX_DISPLAY_PLAYERS] = {false, false, false, false};

lv_color_t get_player_color_vib(int index, int vibrancy)
{
    if (index < 0 || index >= MAX_GAME_PLAYERS) return lv_color_hex(0x303030);
    if (vibrancy < 0 || vibrancy >= LIFE_VIB_COUNT) vibrancy = LIFE_VIB_MID;
    return lv_color_hex(player_color_table[index][vibrancy]);
}

lv_color_t get_player_base_color(int index)
{
    return get_player_color_vib(index, LIFE_VIB_MID);
}

lv_color_t get_player_active_color(int index)
{
    return get_player_color_vib(index, LIFE_VIB_VIV);
}

lv_color_t get_player_text_color(int index)
{
    lv_color_t bg = get_player_base_color(index);
    return color_is_light(bg) ? lv_color_black() : lv_color_white();
}

lv_color_t get_player_preview_color(int index, int delta)
{
    if (index == 3) {
        /* P4 yellow: dark red / dark green for contrast */
        return (delta < 0) ? lv_color_hex(0x7A1020) : lv_color_hex(0x215A2A);
    }
    if (index == 0) {
        /* P1 green: bright red / white for contrast on green bg */
        return (delta < 0) ? lv_palette_main(LV_PALETTE_RED) : lv_color_white();
    }
    return (delta < 0) ? lv_palette_main(LV_PALETTE_RED) : lv_palette_main(LV_PALETTE_GREEN);
}

lv_color_t get_custom_color_vib(int index, int vibrancy)
{
    if (index < 0 || index >= CUSTOM_COLOR_COUNT) index = 0;
    if (vibrancy < 0 || vibrancy >= LIFE_VIB_COUNT) vibrancy = LIFE_VIB_MID;
    return lv_color_hex(custom_color_table[index][vibrancy]);
}

const char *get_custom_color_name(int index)
{
    if (index < 0 || index >= CUSTOM_COLOR_COUNT) index = 0;
    return custom_color_names[index];
}

lv_color_t get_effective_player_color(int player_i, int color_i, int vibrancy)
{
    /* Per-player override takes precedence over global mode */
    if (player_has_override[player_i]) {
        if (player_life_color[player_i]) {
            int life = player_life[player_i];
            int max_life = nvs_get_life_total();
            int tier = get_life_tier(life, max_life);
            return get_life_color_vib(tier, vibrancy);
        }
        return get_custom_color_vib(player_color_index[player_i], vibrancy);
    }

    /* No override: use global mode */
    if (nvs_get_color_mode() == COLOR_MODE_LIFE) {
        int life = player_life[player_i];
        int max_life = nvs_get_life_total();
        int tier = get_life_tier(life, max_life);
        return get_life_color_vib(tier, vibrancy);
    }

    /* COLOR_MODE_PLAYER: use position color */
    return get_player_color_vib(color_i, vibrancy);
}

int get_main_player_index(void)
{
    int num = nvs_get_num_players();
    int i;

    for (i = 0; i < num; i++) {
        if (strcmp(player_names[i], "m") == 0) {
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

const counter_definition_t *get_counter_definition(counter_type_t type)
{
    if (type < 0 || type >= COUNTER_TYPE_COUNT) return NULL;
    return &counter_definitions[type];
}

bool counter_type_is_enabled(counter_type_t type)
{
    const counter_definition_t *definition = get_counter_definition(type);

    return (definition != NULL) && definition->enabled;
}

int get_counter_value(int player, counter_type_t type)
{
    if (player < 0 || player >= MAX_DISPLAY_PLAYERS) return 0;
    if (type < 0 || type >= COUNTER_TYPE_COUNT) return 0;

    return player_counters[player][type];
}

void begin_counter_edit(int player, counter_type_t type)
{
    if (player < 0 || player >= MAX_DISPLAY_PLAYERS) return;
    if (type < 0 || type >= COUNTER_TYPE_COUNT) return;

    menu_player = player;
    counter_edit_type = type;
    counter_edit_value = player_counters[player][type];
}

void change_counter_edit(int delta)
{
    counter_edit_value = clamp_counter(counter_edit_value + delta);
}

int apply_counter_edit(void)
{
    int player = menu_player;
    int old_value;
    int change_delta;

    if (player < 0 || player >= MAX_DISPLAY_PLAYERS) return 0;
    if (counter_edit_type < 0 || counter_edit_type >= COUNTER_TYPE_COUNT) return 0;

    old_value = player_counters[player][counter_edit_type];
    counter_edit_value = clamp_counter(counter_edit_value);
    change_delta = counter_edit_value - old_value;
    player_counters[player][counter_edit_type] = counter_edit_value;

    if (change_delta != 0) {
        damage_log_add(player, change_delta, LOG_EVT_COUNTER, counter_edit_type);
        if (counter_edit_type == COUNTER_TYPE_POISON &&
            old_value < 10 && counter_edit_value >= 10) {
            set_player_elimination_action(player, LOG_EVT_COUNTER, counter_edit_type, change_delta);
        }
        if (counter_edit_type == COUNTER_TYPE_POISON) {
            check_player_elimination(player);
        }
    }

    return change_delta;
}

// ---------- life preview ----------
void life_preview_commit_cb(lv_timer_t *timer)
{
    (void)timer;

    if (!life_preview_active ||
        preview_player < 0 ||
        preview_player >= nvs_get_players_to_track()) {
        if (life_preview_timer != NULL) {
            lv_timer_pause(life_preview_timer);
        }
        return;
    }

    damage_log_add(preview_player, pending_life_delta, LOG_EVT_LIFE, -1);
    player_life[preview_player] = clamp_life(
        player_life[preview_player] + pending_life_delta
    );
    if (player_life[preview_player] <= 0) {
        set_player_elimination_action(preview_player, LOG_EVT_LIFE, -1, pending_life_delta);
    }
    check_player_elimination(preview_player);
    pending_life_delta = 0;
    preview_player = -1;
    life_preview_active = false;
    if (life_preview_timer != NULL) {
        lv_timer_pause(life_preview_timer);
    }
    refresh_player_ui();
}

// ---------- life changes ----------
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

    source = get_cmd_target_player_index(selected_enemy);
    cmd_damage_totals[source][cmd_damage_target] = enemies[selected_enemy].damage;
    damage_log_add(cmd_damage_target, -delta, LOG_EVT_CMD_DAMAGE, source);
    player_life[cmd_damage_target] = clamp_life(player_life[cmd_damage_target] - delta);
    if (cmd_damage_totals[source][cmd_damage_target] >= 20 || player_life[cmd_damage_target] <= 0) {
        set_player_elimination_action(cmd_damage_target, LOG_EVT_CMD_DAMAGE, source, -delta);
    }
    check_player_elimination(cmd_damage_target);

    refresh_select_ui();
}

void damage_cancel(void)
{
    if (selected_enemy >= 0 && selected_enemy < active_enemy_count)
        enemies[selected_enemy].damage = damage_start_value;
}

void change_player_life(int delta)
{
    int preview_base;
    int track = nvs_get_players_to_track();

    if (selected_player < 0 || selected_player >= track) return;

    select_kick_timer();

    if (life_preview_active &&
        preview_player != selected_player) {
        life_preview_commit_cb(NULL);
    }

    preview_player = selected_player;
    preview_base = player_life[preview_player];
    pending_life_delta += delta;
    pending_life_delta = clamp_life(preview_base + pending_life_delta) - preview_base;
    life_preview_active = (pending_life_delta != 0);

    if (life_preview_timer != NULL) {
        lv_timer_reset(life_preview_timer);
    }

    if (!life_preview_active && life_preview_timer != NULL) {
        lv_timer_pause(life_preview_timer);
        preview_player = -1;
    } else if (life_preview_timer != NULL) {
        lv_timer_resume(life_preview_timer);
    }

    refresh_player_ui();
}

void prepare_cmd_damage_for_player(int target)
{
    int i, row = 0;
    int num = nvs_get_num_players();

    cmd_damage_target = target;

    for (i = 0; i < num; i++) {
        if (i == target) continue;
        if (row < MAX_ENEMY_COUNT) {
            enemies[row].damage = cmd_damage_totals[i][target];
            row++;
        }
    }
}

void change_all_damage(int delta)
{
    all_damage_value += delta;
    if (all_damage_value < 0) all_damage_value = 0;
    refresh_all_damage_ui();
}

// ---------- undo ----------
void undo_life_change(int player, int delta)
{
    if (player < 0 || player >= MAX_DISPLAY_PLAYERS) return;

    player_life[player] = clamp_life(player_life[player] - delta);
    check_player_elimination(player);
    refresh_player_ui();
    refresh_select_ui();
}

void undo_cmd_damage(int source, int target, int delta)
{
    if (source < 0 || source >= MAX_GAME_PLAYERS) return;
    if (target < 0 || target >= MAX_DISPLAY_PLAYERS) return;
    cmd_damage_totals[source][target] += delta;
    if (cmd_damage_totals[source][target] < 0)
        cmd_damage_totals[source][target] = 0;
    check_player_elimination(target);
}

void undo_counter_change(int player, int counter_type, int delta)
{
    if (counter_type < 0 || counter_type >= COUNTER_TYPE_COUNT) return;
    if (player < 0 || player >= MAX_DISPLAY_PLAYERS) return;

    player_counters[player][counter_type] = clamp_counter(
        player_counters[player][counter_type] - delta
    );
    if (counter_type == COUNTER_TYPE_POISON) {
        check_player_elimination(player);
    }
    refresh_player_ui();
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

    pending_life_delta = 0;
    preview_player = -1;
    life_preview_active = false;
    selected_enemy = -1;
    dice_result = 0;

    for (i = 0; i < MAX_ENEMY_COUNT; i++) {
        enemies[i].damage = 0;
    }

    for (i = 0; i < MAX_DISPLAY_PLAYERS; i++) {
        player_life[i] = starting_life;
    }
    selected_player = -1;
    menu_player = 0;
    cmd_damage_target = -1;
    memset(cmd_damage_totals, 0, sizeof(cmd_damage_totals));
    memset(player_counters, 0, sizeof(player_counters));
    memset(player_eliminated, 0, sizeof(player_eliminated));
    all_damage_value = 0;
    counter_edit_type = COUNTER_TYPE_COMMANDER_TAX;
    counter_edit_value = 0;

    if (life_preview_timer != NULL) {
        lv_timer_pause(life_preview_timer);
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

    for (i = 0; i < MAX_DISPLAY_PLAYERS; i++) {
        player_life[i] = starting_life;
    }
    memset(player_counters, 0, sizeof(player_counters));
    counter_edit_type = COUNTER_TYPE_COMMANDER_TAX;
    counter_edit_value = 0;

    life_preview_timer = lv_timer_create(life_preview_commit_cb, 3000, NULL);
    if (life_preview_timer != NULL) {
        lv_timer_pause(life_preview_timer);
    }
}

// ---------- player selection animation ----------
static lv_timer_t *player_select_anim_timer = NULL;
static int player_select_anim_steps = 0;
static int player_select_anim_period = 0;

static void player_select_anim_cb(lv_timer_t *timer)
{
    int track = nvs_get_players_to_track();
    (void)timer;

    if (track <= 1) {
        lv_timer_pause(player_select_anim_timer);
        return;
    }

    // Move to next player (clockwise logic mapping to bottom/left/top/right)
    selected_player = (selected_player + 1) % track;
    refresh_player_ui();

    player_select_anim_steps--;
    if (player_select_anim_steps <= 0) {
        lv_timer_pause(player_select_anim_timer);
        select_kick_timer();
    } else {
        // Linear deceleration
        player_select_anim_period += (200 / (player_select_anim_steps + 1));
        if (player_select_anim_period > 600) player_select_anim_period = 600;
        lv_timer_set_period(player_select_anim_timer, player_select_anim_period);
    }
}

void start_player_selection_animation(void)
{
    int track = nvs_get_players_to_track();
    int random_stops;

    if (track <= 1) return;

    if (player_select_anim_timer == NULL) {
        player_select_anim_timer = lv_timer_create(player_select_anim_cb, 50, NULL);
    }

    // Randomize length to ensure random landing
    random_stops = (int)(esp_random() % track) + (track * 3);
    random_stops += esp_random() % (track * 2);

    player_select_anim_steps = random_stops;
    player_select_anim_period = 40; // start fast

    if (selected_player < 0) selected_player = 0;

    lv_timer_set_period(player_select_anim_timer, player_select_anim_period);
    lv_timer_resume(player_select_anim_timer);
}
