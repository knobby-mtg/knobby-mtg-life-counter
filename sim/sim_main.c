#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "sim_stubs.h"
#include "board_detect.h"
#include <lvgl.h>
#include "knob.h"
#include "hw.h"
#include "storage.h"
#include "game.h"
#include "ui_1p.h"
#include "ui_mp.h"
#include "ui_player_menu.h"
#include "settings.h"
#include "intro.h"
#include "dice.h"
#include "timer.h"
#include "damage_log.h"
#include "game_mode.h"
#include "rename.h"
#include "mana.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SCREEN_W 360
#define SCREEN_H 360

/* Globals normally defined in scr_st77916.h */
lv_indev_t *indev_knob = NULL;
int encoder_cont = 0;

/* ---- Framebuffer ---- */
static lv_color_t framebuffer[SCREEN_W * SCREEN_H];
static lv_color_t draw_buf_data[SCREEN_W * 72];
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;

static void sim_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    int x, y;
    for (y = area->y1; y <= area->y2; y++) {
        for (x = area->x1; x <= area->x2; x++) {
            framebuffer[y * SCREEN_W + x] = *color_p++;
        }
    }
    lv_disp_flush_ready(drv);
}

static void save_screenshot_png(const char *filename)
{
    uint8_t *rgb = malloc(SCREEN_W * SCREEN_H * 3);
    if (!rgb) { fprintf(stderr, "Failed to allocate RGB buffer\n"); return; }

    {
        const int cx = SCREEN_W / 2;
        const int cy = SCREEN_H / 2;
        const int r  = SCREEN_W / 2;
        const int r2 = r * r;
        int x, y;

        for (y = 0; y < SCREEN_H; y++) {
            for (x = 0; x < SCREEN_W; x++) {
                int idx = y * SCREEN_W + x;
                int dx = x - cx;
                int dy = y - cy;
                if (dx * dx + dy * dy > r2) {
                    rgb[idx * 3 + 0] = 0xE0;
                    rgb[idx * 3 + 1] = 0xE0;
                    rgb[idx * 3 + 2] = 0xE0;
                } else {
                    lv_color_t c = framebuffer[idx];
                    rgb[idx * 3 + 0] = (uint8_t)((c.ch.red * 255) / 31);
                    rgb[idx * 3 + 1] = (uint8_t)((c.ch.green * 255) / 63);
                    rgb[idx * 3 + 2] = (uint8_t)((c.ch.blue * 255) / 31);
                }
            }
        }
    }

    if (!stbi_write_png(filename, SCREEN_W, SCREEN_H, 3, rgb, SCREEN_W * 3)) {
        fprintf(stderr, "Failed to write %s\n", filename);
    } else {
        printf("Saved: %s\n", filename);
    }
    free(rgb);
}

static void render_frame(void)
{
    int i;
    for (i = 0; i < 20; i++) {
        sim_tick_advance(30);
        lv_timer_handler();
    }
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(NULL);
}

/* ---- Screen navigation ---- */
extern void reset_all_values(void);

typedef struct {
    const char *name;
    void (*navigate)(void);
} screen_entry_t;

static void nav_main(void)       { back_to_main(); }
static void nav_1p(void)         { nvs_set_players_to_track(1); reset_all_values(); back_to_main(); }
static void nav_2p(void)         { nvs_set_players_to_track(2); reset_all_values(); back_to_main(); }
static void nav_3p(void)         { nvs_set_players_to_track(3); reset_all_values(); back_to_main(); }
static void nav_4p(void)         { nvs_set_players_to_track(4); reset_all_values(); back_to_main(); }
static void nav_intro(void)      { lv_scr_load(screen_intro); }
static void nav_menu(void)       { open_quad_menu(); }
static void nav_tools(void)      { lv_scr_load(screen_tools_menu); }
static void nav_settings_menu(void) { lv_scr_load(screen_screen_settings_menu); }
static void nav_settings_more(void) { lv_scr_load(screen_settings_page2); }
static void nav_brightness(void) { open_settings_screen(); }
static void nav_battery(void)    { update_battery_measurement(true); refresh_battery_ui(); lv_scr_load(screen_battery); }
static void nav_dice(void)       { open_dice_screen(); }
static void nav_damage_log(void) { open_damage_log_screen(); }
static void nav_game_mode(void)  { open_game_mode_menu(); }
static void nav_custom_life(void){ open_game_mode_menu(); lv_scr_load(screen_custom_life); refresh_custom_life_ui(); }
static void nav_select(void)     { open_select_screen(); }
static void nav_damage(void) {
    selected_enemy = 0;
    damage_enter();
    refresh_damage_ui();
    lv_scr_load(screen_damage);
}
static void nav_player_menu(void)  { open_player_menu(0); }
static void nav_rename(void)       { open_rename_screen(); }
static void nav_all_damage(void) {
    all_damage_value = 5;
    refresh_all_damage_ui();
    lv_scr_load(screen_player_all_damage);
}
static void nav_counters_menu(void) { lv_scr_load(screen_counter_menu); }
static void nav_counter_edit(void) {
    begin_counter_edit(0, COUNTER_TYPE_POISON);
    refresh_counter_edit_ui();
    lv_scr_load(screen_counter_edit);
}
static void nav_color_menu(void)   { load_screen_if_needed(screen_player_color_menu); }
static void nav_color_picker(void) {
    player_color_index[menu_player] = 5;  /* show Orange as example */
    load_screen_if_needed(screen_player_color_picker);
}
static void nav_mana(void) { open_mana_screen(); }

static const screen_entry_t all_screens[] = {
    {"main",          nav_main},
    {"1p",            nav_1p},
    {"2p",            nav_2p},
    {"3p",            nav_3p},
    {"4p",            nav_4p},
    {"intro",         nav_intro},
    {"menu",          nav_menu},
    {"tools",         nav_tools},
    {"settings-menu", nav_settings_menu},
    {"settings-more", nav_settings_more},
    {"brightness",    nav_brightness},
    {"battery",       nav_battery},
    {"dice",          nav_dice},
    {"damage-log",    nav_damage_log},
    {"game-mode",     nav_game_mode},
    {"custom-life",   nav_custom_life},
    {"select",        nav_select},
    {"damage",        nav_damage},
    {"player-menu",   nav_player_menu},
    {"rename",        nav_rename},
    {"all-damage",    nav_all_damage},
    {"counters-menu", nav_counters_menu},
    {"counter-edit",  nav_counter_edit},
    {"color-menu",    nav_color_menu},
    {"color-picker",  nav_color_picker},
    {"mana",          nav_mana},
    {NULL, NULL}
};

/* ---- Usage ---- */
static void print_usage(void)
{
    printf("Usage: knobby_sim [options]\n"
           "\nScreen selection:\n"
           "  --screen <name>        Screen to capture (default: main)\n"
           "  (use generate_matrix.sh to screenshot all screens)\n"
           "  --outdir <path>        Output directory (default: screenshots)\n"
           "  --output <filename>    Output filename (overrides default naming)\n"
           "\nGame state:\n"
           "  --life <p1,p2,...>     Set player life totals (default: starting-life for all)\n"
           "  --names <p1,p2,...>    Set player names (default: P1..P8)\n"
           "  --players <n>          Number of players in the game, 1-8 (default: 4)\n"
           "  --track <n>            Players shown on screen, 1-4 (default: 4)\n"
           "  --starting-life <n>    Starting/max life total (default: 40)\n"
           "  --selected <n>         Which player is selected, -1=none (default: -1)\n"
           "\nLife preview (shows pending delta before commit):\n"
           "  --preview-delta <n>    Pending life change to display (e.g. +12, -5)\n"
           "  --preview-player <n>   Which player shows the preview, -1=1p mode (default: -1)\n"
           "\nDisplay settings:\n"
           "  --color-mode <n>       0=player, 1=life (default: 0)\n"
           "  --player-colors <csv>  Per-player custom color indices, e.g. 0,5,2,13\n"
           "  --player-override <csv> Per-player override flags, e.g. 0,1,0,0\n"
           "  --orientation <n>      0=absolute, 1=centric, 2=tabletop (default: 0)\n"
           "  --brightness <n>       Brightness percent 1-100 (default: 30)\n"
           "  --auto-dim <n>         0=OFF, 1=15s, 2=30s, 3=60s (default: 0)\n"
           "  --deselect <n>         0=never, 1=5s, 2=15s, 3=30s (default: 0)\n"
           "  --auto-eliminate <n>   0=OFF, 1=ON (default: 1)\n"
           "\nSpecial state:\n"
           "  --dice <n>             Set dice roll result (1-20)\n"
           "  --counter-type <n>     Counter type for counter-edit: 0=cmd tax, 1=partner tax,\n"
           "                         2=poison, 3=experience (default: 2)\n"
           "  --counter-value <n>    Counter value for counter-edit (default: 0)\n"
           "  --counter-player <n>   Player for counter-edit, 0-3 (default: 0)\n"
           "  --enemy-damage <csv>   Set commander damage per enemy row (e.g. 5,12,3)\n"
           "  --all-damage-value <n> Value for all-damage screen (default: 5)\n"
           "  --menu-player <n>      Player index for player-menu, 0-3 (default: 0)\n"
           "  --battery-voltage <f>  Battery voltage for battery screen (default: 4.0)\n"
           "  --random-counters      Set all player counters to random values 0-99\n"
           "  --random-log           Populate event log with random entries\n"
           "\nMana pool:\n"
           "  --mana <W,U,B,R,G,C>  Set mana pool values (e.g. 3,1,0,2,0,5)\n"
           "  --mana-selected <n>    Selected mana color, -1=none (default: -1)\n"
           "  --mana-delta <n>       Pending mana delta for preview display (e.g. +5, -3)\n"
           "\nTimer state (1p only):\n"
           "  --turn-number <n>      Turn number (enables timer display when > 0)\n"
           "  --turn-elapsed <ms>    Elapsed game time in milliseconds\n"
           "\n  --help, -h             Show this message\n"
           "\nAvailable screens:\n"
           "  main 1p 2p 3p 4p intro menu tools settings-menu settings-more\n"
           "  brightness battery dice damage-log game-mode custom-life select\n"
           "  damage player-menu rename all-damage counters-menu counter-edit\n"
           "  color-menu color-picker mana\n");
}

/* ---- CSV helpers ---- */
static void parse_csv_ints(const char *csv, int *out, int max_count)
{
    char buf[256];
    char *tok;
    int i = 0;
    snprintf(buf, sizeof(buf), "%s", csv);
    tok = strtok(buf, ",");
    while (tok && i < max_count) {
        out[i++] = atoi(tok);
        tok = strtok(NULL, ",");
    }
}

static void parse_csv_strings(const char *csv, char names[][16], int max_count)
{
    char buf[256];
    char *tok;
    int i = 0;
    snprintf(buf, sizeof(buf), "%s", csv);
    tok = strtok(buf, ",");
    while (tok && i < max_count) {
        snprintf(names[i], 16, "%s", tok);
        i++;
        tok = strtok(NULL, ",");
    }
}

/* ---- Random data generators ---- */
static void populate_random_counters(void)
{
    int p, t;
    for (p = 0; p < MAX_DISPLAY_PLAYERS; p++)
        for (t = 0; t < COUNTER_TYPE_COUNT; t++)
            player_counters[p][t] = rand() % 100;
}

static void populate_random_log(void)
{
    int i;
    const uint8_t event_types[] = {LOG_EVT_LIFE, LOG_EVT_CMD_DAMAGE, LOG_EVT_COUNTER};
    for (i = 0; i < 15; i++) {
        int player = rand() % 4;
        int delta = (rand() % 20) - 10;
        uint8_t evt = event_types[rand() % 3];
        int source = -1;
        if (delta == 0) delta = 1;
        if (evt == LOG_EVT_CMD_DAMAGE) source = (player + 1) % 4;
        else if (evt == LOG_EVT_COUNTER) { source = rand() % COUNTER_TYPE_COUNT; if (delta < 0) delta = -delta; }
        sim_tick_advance(5000 + (uint32_t)(rand() % 30000));
        damage_log_add(player, delta, evt, source);
    }
}

/* ---- Main ---- */
int main(int argc, char *argv[])
{
    const char *screen_name = "main";
    const char *outdir = "screenshots";
    const char *output_filename = NULL;
    int life_values[MAX_DISPLAY_PLAYERS] = {0};
    int life_set = 0;
    char name_values[MAX_GAME_PLAYERS][16] = {{0}};
    int names_set = 0;
    int selected_val = -1;
    int selected_set = 0;
    int preview_delta = 0;
    int preview_delta_set = 0;
    int arg_preview_player = -1;
    int dice_val = 0;
    int dice_set = 0;
    int counter_type_val = 2; /* default: poison */
    int counter_type_set = 0;
    int counter_value_val = 0;
    int counter_value_set = 0;
    int counter_player_val = 0;
    int enemy_damage_values[MAX_ENEMY_COUNT] = {0};
    int enemy_damage_set = 0;
    int all_damage_val = 5;
    int all_damage_set = 0;
    int menu_player_val = 0;
    int menu_player_set = 0;
    int player_color_values[MAX_DISPLAY_PLAYERS] = {0, 1, 2, 3};
    int player_colors_set = 0;
    int player_override_values[MAX_DISPLAY_PLAYERS] = {0, 0, 0, 0};
    int player_override_set = 0;
    int brightness_val = 0;
    int brightness_set = 0;
    int do_random_counters = 0;
    int do_random_log = 0;
    int turn_number_val = 0;
    int turn_number_set = 0;
    uint32_t turn_elapsed_val = 0;
    int turn_elapsed_set = 0;
    int mana_vals[MANA_COLOR_COUNT] = {0};
    int mana_set = 0;
    int mana_sel_val = -1;
    int mana_sel_set = 0;
    int mana_delta_val = 0;
    int mana_delta_set = 0;
    int i;

    srand((unsigned int)time(NULL));

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--screen") == 0 && i + 1 < argc) {
            screen_name = argv[++i];
        } else if (strcmp(argv[i], "--life") == 0 && i + 1 < argc) {
            parse_csv_ints(argv[++i], life_values, MAX_DISPLAY_PLAYERS);
            life_set = 1;
        } else if (strcmp(argv[i], "--names") == 0 && i + 1 < argc) {
            parse_csv_strings(argv[++i], name_values, MAX_GAME_PLAYERS);
            names_set = 1;
        } else if (strcmp(argv[i], "--players") == 0 && i + 1 < argc) {
            sim_nvs_preset_i8("num_players", (int8_t)atoi(argv[++i]));
        } else if (strcmp(argv[i], "--track") == 0 && i + 1 < argc) {
            sim_nvs_preset_i8("track", (int8_t)atoi(argv[++i]));
        } else if (strcmp(argv[i], "--starting-life") == 0 && i + 1 < argc) {
            sim_nvs_preset_i16("life_total", (int16_t)atoi(argv[++i]));
        } else if (strcmp(argv[i], "--selected") == 0 && i + 1 < argc) {
            selected_val = atoi(argv[++i]);
            selected_set = 1;
        } else if (strcmp(argv[i], "--preview-delta") == 0 && i + 1 < argc) {
            preview_delta = atoi(argv[++i]);
            preview_delta_set = 1;
        } else if (strcmp(argv[i], "--preview-player") == 0 && i + 1 < argc) {
            arg_preview_player = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--color-mode") == 0 && i + 1 < argc) {
            sim_nvs_preset_i8("color_mode", (int8_t)atoi(argv[++i]));
        } else if (strcmp(argv[i], "--orientation") == 0 && i + 1 < argc) {
            sim_nvs_preset_i8("rotation", (int8_t)atoi(argv[++i]));
        } else if (strcmp(argv[i], "--brightness") == 0 && i + 1 < argc) {
            brightness_val = atoi(argv[++i]);
            brightness_set = 1;
        } else if (strcmp(argv[i], "--auto-dim") == 0 && i + 1 < argc) {
            sim_nvs_preset_i8("auto_dim", (int8_t)atoi(argv[++i]));
        } else if (strcmp(argv[i], "--deselect") == 0 && i + 1 < argc) {
            sim_nvs_preset_i8("desel_time", (int8_t)atoi(argv[++i]));
        } else if (strcmp(argv[i], "--auto-eliminate") == 0 && i + 1 < argc) {
            sim_nvs_preset_i8("auto_elim", (int8_t)atoi(argv[++i]));
        } else if (strcmp(argv[i], "--dice") == 0 && i + 1 < argc) {
            dice_val = atoi(argv[++i]);
            dice_set = 1;
        } else if (strcmp(argv[i], "--counter-type") == 0 && i + 1 < argc) {
            counter_type_val = atoi(argv[++i]);
            counter_type_set = 1;
        } else if (strcmp(argv[i], "--counter-value") == 0 && i + 1 < argc) {
            counter_value_val = atoi(argv[++i]);
            counter_value_set = 1;
        } else if (strcmp(argv[i], "--counter-player") == 0 && i + 1 < argc) {
            counter_player_val = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--enemy-damage") == 0 && i + 1 < argc) {
            parse_csv_ints(argv[++i], enemy_damage_values, MAX_ENEMY_COUNT);
            enemy_damage_set = 1;
        } else if (strcmp(argv[i], "--all-damage-value") == 0 && i + 1 < argc) {
            all_damage_val = atoi(argv[++i]);
            all_damage_set = 1;
        } else if (strcmp(argv[i], "--menu-player") == 0 && i + 1 < argc) {
            menu_player_val = atoi(argv[++i]);
            menu_player_set = 1;
        } else if (strcmp(argv[i], "--player-colors") == 0 && i + 1 < argc) {
            parse_csv_ints(argv[++i], player_color_values, MAX_DISPLAY_PLAYERS);
            player_colors_set = 1;
        } else if (strcmp(argv[i], "--player-override") == 0 && i + 1 < argc) {
            parse_csv_ints(argv[++i], player_override_values, MAX_DISPLAY_PLAYERS);
            player_override_set = 1;
        } else if (strcmp(argv[i], "--battery-voltage") == 0 && i + 1 < argc) {
            sim_battery_voltage = (float)atof(argv[++i]);
        } else if (strcmp(argv[i], "--random-counters") == 0) {
            do_random_counters = 1;
        } else if (strcmp(argv[i], "--random-log") == 0) {
            do_random_log = 1;
        } else if (strcmp(argv[i], "--turn-number") == 0 && i + 1 < argc) {
            turn_number_val = atoi(argv[++i]);
            turn_number_set = 1;
        } else if (strcmp(argv[i], "--turn-elapsed") == 0 && i + 1 < argc) {
            turn_elapsed_val = (uint32_t)atol(argv[++i]);
            turn_elapsed_set = 1;
        } else if (strcmp(argv[i], "--mana") == 0 && i + 1 < argc) {
            parse_csv_ints(argv[++i], mana_vals, MANA_COLOR_COUNT);
            mana_set = 1;
        } else if (strcmp(argv[i], "--mana-selected") == 0 && i + 1 < argc) {
            mana_sel_val = atoi(argv[++i]);
            mana_sel_set = 1;
        } else if (strcmp(argv[i], "--mana-delta") == 0 && i + 1 < argc) {
            mana_delta_val = atoi(argv[++i]);
            mana_delta_set = 1;
        } else if (strcmp(argv[i], "--outdir") == 0 && i + 1 < argc) {
            outdir = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_filename = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage();
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage();
            return 1;
        }
    }

    /* Initialize */
    board_detect();
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, draw_buf_data, NULL, SCREEN_W * 72);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_W;
    disp_drv.ver_res = SCREEN_H;
    disp_drv.flush_cb = sim_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    knob_gui();

    /* Apply RAM-only overrides after navigation */
    #define APPLY_RAM_OVERRIDES() do { \
        if (life_set) { \
            for (i = 0; i < MAX_DISPLAY_PLAYERS; i++) \
                player_life[i] = life_values[i]; \
        } \
        if (names_set) { \
            for (i = 0; i < MAX_GAME_PLAYERS; i++) { \
                if (name_values[i][0]) \
                    snprintf(player_names[i], sizeof(player_names[i]), "%s", name_values[i]); \
            } \
        } \
        if (selected_set) \
            selected_player = selected_val; \
        if (preview_delta_set) { \
            if (arg_preview_player < 0) arg_preview_player = 0; \
            preview_player = arg_preview_player; \
            pending_life_delta = preview_delta; \
            life_preview_active = true; \
            selected_player = arg_preview_player; \
        } \
        if (brightness_set) { \
            brightness_percent = brightness_val; \
            brightness_apply(); \
        } \
        if (dice_set) { \
            dice_result = dice_val; \
            refresh_dice_ui(); \
        } \
        if (counter_type_set || counter_value_set) { \
            begin_counter_edit(counter_player_val, (counter_type_t)counter_type_val); \
            if (counter_value_set) counter_edit_value = counter_value_val; \
            refresh_counter_edit_ui(); \
        } \
        if (enemy_damage_set) { \
            for (i = 0; i < MAX_ENEMY_COUNT; i++) \
                enemies[i].damage = enemy_damage_values[i]; \
            refresh_select_ui(); \
            refresh_damage_ui(); \
        } \
        if (all_damage_set) { \
            all_damage_value = all_damage_val; \
            refresh_all_damage_ui(); \
        } \
        if (menu_player_set) { \
            menu_player = menu_player_val; \
        } \
        if (player_colors_set) { \
            for (i = 0; i < MAX_DISPLAY_PLAYERS; i++) \
                player_color_index[i] = player_color_values[i]; \
        } \
        if (player_override_set) { \
            for (i = 0; i < MAX_DISPLAY_PLAYERS; i++) \
                player_has_override[i] = (bool)player_override_values[i]; \
        } \
        if (do_random_counters) \
            populate_random_counters(); \
        if (do_random_log) { \
            populate_random_log(); \
            open_damage_log_screen(); \
        } \
        if (turn_number_set) { \
            turn_number = turn_number_val; \
            turn_timer_enabled = (turn_number_val > 0); \
            turn_ui_visible = (turn_number_val > 0); \
            turn_started_ms = lv_tick_get(); \
            if (turn_elapsed_set) \
                turn_elapsed_ms = turn_elapsed_val; \
            else \
                turn_elapsed_ms = 0; \
        } \
        if (mana_set) { \
            for (i = 0; i < MANA_COLOR_COUNT; i++) \
                mana_values[i] = mana_vals[i]; \
        } \
        if (mana_sel_set) \
            mana_set_selected(mana_sel_val); \
        if (mana_delta_set && mana_sel_set && mana_sel_val >= 0) { \
            for (int md = 0; md < (mana_delta_val > 0 ? mana_delta_val : -mana_delta_val); md++) \
                change_mana_value(mana_delta_val > 0 ? 1 : -1); \
        } \
        for (i = 0; i < MAX_DISPLAY_PLAYERS; i++) \
            check_player_elimination(i); \
        refresh_main_ui(); \
        lv_obj_update_layout(lv_scr_act()); \
        refresh_multiplayer_ui(); \
        refresh_select_ui(); \
        refresh_damage_ui(); \
        refresh_settings_ui(); \
        refresh_battery_ui(); \
        refresh_mana_ui(); \
    } while(0)

    {
        const screen_entry_t *entry;
        int found = 0;
        for (entry = all_screens; entry->name != NULL; entry++) {
            if (strcmp(entry->name, screen_name) == 0) {
                char path[512];
                entry->navigate();
                APPLY_RAM_OVERRIDES();
                render_frame();
                if (output_filename)
                    snprintf(path, sizeof(path), "%s/%s", outdir, output_filename);
                else
                    snprintf(path, sizeof(path), "%s/%s.png", outdir, screen_name);
                save_screenshot_png(path);
                found = 1;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "Unknown screen: %s\n", screen_name);
            print_usage();
            return 1;
        }
    }

    return 0;
}
