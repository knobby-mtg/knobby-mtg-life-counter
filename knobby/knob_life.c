#include "knob_life.h"
#include "knob_nvs.h"
#include "knob_damage_log.h"

// Forward declarations for UI refresh (defined in screen modules)
extern void refresh_main_ui(void);
extern void refresh_select_ui(void);
extern void refresh_damage_ui(void);
extern void refresh_multiplayer_ui(void);
extern void refresh_multiplayer_all_damage_ui(void);
extern void select_kick_timer(void);

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

int multiplayer_life[MAX_PLAYERS] = {40, 40, 40, 40};
int multiplayer_selected = -1;
char multiplayer_names[MAX_PLAYERS][16] = {"P1", "P2", "P3", "P4"};
int multiplayer_menu_player = 0;
int multiplayer_cmd_damage_totals[MAX_PLAYERS][MAX_PLAYERS] = {{0}};
int multiplayer_all_damage_value = 0;
int cmd_damage_target = -1;
static int damage_start_value = 0;
int multiplayer_pending_life_delta = 0;
int multiplayer_preview_player = -1;
bool multiplayer_life_preview_active = false;
int multiplayer_counter_values[MAX_PLAYERS][COUNTER_TYPE_COUNT] = {{0}};
counter_type_t multiplayer_counter_edit_type = COUNTER_TYPE_COMMANDER_TAX;
int multiplayer_counter_edit_value = 0;
static lv_timer_t *life_preview_timer = NULL;
static lv_timer_t *multiplayer_life_preview_timer = NULL;

static const counter_definition_t counter_definitions[COUNTER_TYPE_COUNT] = {
    {"Commander\nTax", "Commander Tax", "C", 0xA84300, true},
    {"Partner\nTax", "Partner Tax", "P", 0x1565C0, true},
    {"Poison", "Poison", "!", 0x2E7D32, true},
    {"Experience", "Experience", "E", 0x6A1B9A, true},
};

// ---------- player colors ----------
static const uint32_t player_color_table[MAX_PLAYERS][LIFE_VIB_COUNT] = {
    /*  dim        mid        vivid  */
    {0x1A3D1A, 0xA5D6A7, 0x4CAF50},  /* P1 green  (bottom-left) */
    {0x2A0A4D, 0x7B1FE0, 0x9C4DFF},  /* P2 purple (top-left)    */
    {0x0A3A4D, 0x29B6F6, 0x4FC3F7},  /* P3 blue   (top-right)   */
    {0x4D4400, 0xFFD600, 0xFFEA61},  /* P4 yellow (bottom-right) */
};

lv_color_t get_player_color_vib(int index, int vibrancy)
{
    if (index < 0 || index >= MAX_PLAYERS) return lv_color_hex(0x303030);
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
    return (index == 3) ? lv_color_black() : lv_color_white();
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

int get_multiplayer_counter_value(int player, counter_type_t type)
{
    if (player < 0 || player >= MAX_PLAYERS) return 0;
    if (type < 0 || type >= COUNTER_TYPE_COUNT) return 0;

    return multiplayer_counter_values[player][type];
}

void begin_multiplayer_counter_edit(int player, counter_type_t type)
{
    if (player < 0 || player >= MAX_PLAYERS) return;
    if (type < 0 || type >= COUNTER_TYPE_COUNT) return;

    multiplayer_menu_player = player;
    multiplayer_counter_edit_type = type;
    multiplayer_counter_edit_value = multiplayer_counter_values[player][type];
}

void change_multiplayer_counter_edit(int delta)
{
    multiplayer_counter_edit_value = clamp_counter(multiplayer_counter_edit_value + delta);
}

int apply_multiplayer_counter_edit(void)
{
    int player = multiplayer_menu_player;
    int old_value;
    int change_delta;

    if (player < 0 || player >= MAX_PLAYERS) return 0;
    if (multiplayer_counter_edit_type < 0 || multiplayer_counter_edit_type >= COUNTER_TYPE_COUNT) return 0;

    old_value = multiplayer_counter_values[player][multiplayer_counter_edit_type];
    multiplayer_counter_edit_value = clamp_counter(multiplayer_counter_edit_value);
    change_delta = multiplayer_counter_edit_value - old_value;
    multiplayer_counter_values[player][multiplayer_counter_edit_type] = multiplayer_counter_edit_value;

    if (change_delta != 0) {
        damage_log_add(player, change_delta, LOG_EVT_COUNTER, multiplayer_counter_edit_type);
    }

    return change_delta;
}

// ---------- life preview ----------
static void life_preview_commit_cb(lv_timer_t *timer)
{
    (void)timer;

    damage_log_add(-1, pending_life_delta, LOG_EVT_LIFE, -1);
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

    damage_log_add(multiplayer_preview_player, multiplayer_pending_life_delta, LOG_EVT_LIFE, -1);
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
        damage_log_add(cmd_damage_target, -delta, LOG_EVT_CMD_DAMAGE, source);
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

    select_kick_timer();

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

// ---------- undo ----------
void undo_life_change(int player, int delta)
{
    if (player < 0) {
        life_total = clamp_life(life_total - delta);
        refresh_main_ui();
        refresh_select_ui();
    } else if (player < MAX_PLAYERS) {
        multiplayer_life[player] = clamp_life(multiplayer_life[player] - delta);
        refresh_multiplayer_ui();
    }
}

void undo_cmd_damage(int source, int target, int delta)
{
    if (source < 0 || source >= MAX_PLAYERS) return;
    if (target < 0 || target >= MAX_PLAYERS) return;
    multiplayer_cmd_damage_totals[source][target] += delta;
    if (multiplayer_cmd_damage_totals[source][target] < 0)
        multiplayer_cmd_damage_totals[source][target] = 0;
}

void undo_counter_change(int player, int counter_type, int delta)
{
    if (player < 0 || player >= MAX_PLAYERS) return;
    if (counter_type < 0 || counter_type >= COUNTER_TYPE_COUNT) return;

    multiplayer_counter_values[player][counter_type] = clamp_counter(
        multiplayer_counter_values[player][counter_type] - delta
    );
    refresh_multiplayer_ui();
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
    multiplayer_selected = -1;
    multiplayer_menu_player = 0;
    cmd_damage_target = -1;
    memset(multiplayer_cmd_damage_totals, 0, sizeof(multiplayer_cmd_damage_totals));
    memset(multiplayer_counter_values, 0, sizeof(multiplayer_counter_values));
    multiplayer_all_damage_value = 0;
    multiplayer_counter_edit_type = COUNTER_TYPE_COMMANDER_TAX;
    multiplayer_counter_edit_value = 0;

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
    memset(multiplayer_counter_values, 0, sizeof(multiplayer_counter_values));
    multiplayer_counter_edit_type = COUNTER_TYPE_COMMANDER_TAX;
    multiplayer_counter_edit_value = 0;

    life_preview_timer = lv_timer_create(life_preview_commit_cb, 3000, NULL);
    if (life_preview_timer != NULL) {
        lv_timer_pause(life_preview_timer);
    }
    multiplayer_life_preview_timer = lv_timer_create(multiplayer_life_preview_commit_cb, 3000, NULL);
    if (multiplayer_life_preview_timer != NULL) {
        lv_timer_pause(multiplayer_life_preview_timer);
    }
}
