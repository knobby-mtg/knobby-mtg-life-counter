#include "knob_types.h"
#include "knob_hw.h"
#include "knob_nvs.h"
#include "knob_life.h"
#include "knob_timer.h"
#include "knob_dice.h"
#include "knob_intro.h"
#include "knob_scr_main.h"
#include "knob_scr_multiplayer.h"
#include "knob_scr_menus.h"
#include "knob_game_mode.h"
#include "knob_damage_log.h"
#include "knob_rename.h"

// ---------- swipe state ----------
static lv_obj_t *previous_screen = NULL;
static volatile bool swipe_up_pending = false;
static volatile bool swipe_down_pending = false;
static volatile bool swipe_left_pending = false;
static volatile bool swipe_right_pending = false;

// ---------- knob event queue ----------
static int last_knob_cont = 0;
static bool knob_initialized = false;
static knob_input_event_t knob_event_queue[KNOB_EVENT_QUEUE_SIZE];
static volatile uint8_t knob_event_head = 0;
static volatile uint8_t knob_event_tail = 0;

// ---------- swipe notifications ----------
void knob_notify_swipe_up(void)
{
    swipe_up_pending = true;
}

void knob_notify_swipe_down(void)
{
    swipe_down_pending = true;
}

void knob_notify_swipe_left(void)
{
    swipe_left_pending = true;
}

void knob_notify_swipe_right(void)
{
    swipe_right_pending = true;
}

static bool is_player_screen(lv_obj_t *screen)
{
    return screen == screen_1p ||
           screen == screen_2p ||
           screen == screen_3p ||
           screen == screen_4p;
}

static void open_menu_for_screen(lv_obj_t *screen)
{
    if (is_player_screen(screen)) {
        previous_screen = screen;
        open_quad_menu();
    }
}

static void handle_back_navigation(lv_obj_t *screen)
{
    if (screen == screen_quad_menu && previous_screen != NULL) {
        refresh_multiplayer_ui();
        lv_scr_load(previous_screen);
    } else if (screen == screen_tools_menu) {
        lv_scr_load(screen_quad_menu);
    } else if (screen == screen_screen_settings_menu) {
        settings_save();
        lv_scr_load(screen_quad_menu);
    } else if (screen == screen_settings_page2) {
        lv_scr_load(screen_screen_settings_menu);
    } else if (screen == screen_settings) {
        lv_scr_load(screen_screen_settings_menu);
    } else if (screen == screen_battery) {
        lv_scr_load(screen_screen_settings_menu);
    } else if (screen == screen_dice) {
        lv_scr_load(screen_tools_menu);
    } else if (screen == screen_damage_log) {
        lv_scr_load(screen_tools_menu);
    } else if (screen == screen_select) {
        back_to_main();
    } else if (screen == screen_damage) {
        damage_cancel();
        open_select_screen();
    } else if (screen == screen_game_mode_menu) {
        lv_scr_load(screen_quad_menu);
    } else if (screen == screen_custom_life) {
        refresh_game_mode_menu_ui();
        lv_scr_load(screen_game_mode_menu);
    } else if (screen == screen_player_menu) {
        open_multiplayer_screen();
    } else if (screen == screen_player_name) {
        if (!name_screen_handle_back())
            open_multiplayer_menu_screen(multiplayer_menu_player);
    } else if (screen == screen_player_counters_menu) {
        open_multiplayer_menu_screen(multiplayer_menu_player);
    } else if (screen == screen_player_counter_edit) {
        open_multiplayer_counter_menu_screen();
    } else if (screen == screen_player_all_damage) {
        open_multiplayer_menu_screen(multiplayer_menu_player);
    }
}

// ---------- reset ----------
void reset_all_values(void)
{
    knob_life_reset();

    brightness_percent = nvs_get_brightness();
    brightness_apply();

    turn_timer_reset();

    refresh_main_ui();
    refresh_select_ui();
    refresh_damage_ui();
    refresh_settings_ui();
    refresh_multiplayer_ui();

    refresh_rename_ui();
    refresh_multiplayer_all_damage_ui();
    refresh_multiplayer_counter_edit_ui();
}

void knob_cb(lv_event_t *e)
{
    (void)e;
}

// ---------- init ----------
void knob_gui(void)
{
    knob_hw_init();

    build_intro_screen();
    lv_scr_load(screen_intro);
    lv_refr_now(NULL);
    brightness_apply();
    build_dice_screen();
    build_main_screen();
    build_multiplayer_screen();
    build_multiplayer_2p_screen();
    build_multiplayer_3p_screen();
    build_multiplayer_menu_screen();
    build_rename_screen();
    build_multiplayer_all_damage_screen();
    build_multiplayer_counter_menu_screen();
    build_multiplayer_counter_edit_screen();
    build_select_screen();
    build_damage_screen();
    build_settings_screen();
    build_battery_screen();
    build_damage_log_screen();
    build_quad_menus();
    build_game_mode_menu_screen();
    build_custom_life_screen();

    refresh_main_ui();
    refresh_multiplayer_ui();

    refresh_rename_ui();
    refresh_select_ui();
    refresh_damage_ui();
    refresh_multiplayer_all_damage_ui();
    refresh_multiplayer_counter_edit_ui();
    refresh_settings_ui();

    knob_timer_init();
    knob_life_init();
    knob_intro_init();
}

// ---------- knob event handler ----------
static void handle_knob_event(knob_event_t k)
{
    activity_kick();
    if (in_undim_grace()) return;

    if (lv_scr_act() == screen_intro)
    {
        return;
    }
    else if (lv_scr_act() == screen_1p)
    {
        if (k == KNOB_LEFT)      change_life(-1);
        else if (k == KNOB_RIGHT) change_life(+1);
    }
    else if (lv_scr_act() == screen_damage)
    {
        if (k == KNOB_LEFT)      add_damage_to_selected_enemy(-1);
        else if (k == KNOB_RIGHT) add_damage_to_selected_enemy(+1);
    }
    else if (lv_scr_act() == screen_settings)
    {
        if (k == KNOB_LEFT)      change_brightness(-1);
        else if (k == KNOB_RIGHT) change_brightness(+1);
        refresh_settings_ui();
    }
    else if (lv_scr_act() == screen_4p ||
             lv_scr_act() == screen_3p ||
             lv_scr_act() == screen_2p)
    {
        if (k == KNOB_LEFT)      change_multiplayer_life(-1);
        else if (k == KNOB_RIGHT) change_multiplayer_life(+1);
    }
    else if (lv_scr_act() == screen_player_all_damage)
    {
        if (k == KNOB_LEFT)      change_multiplayer_all_damage(-1);
        else if (k == KNOB_RIGHT) change_multiplayer_all_damage(+1);
    }
    else if (lv_scr_act() == screen_custom_life)
    {
        if (k == KNOB_LEFT)      change_custom_life(-1);
        else if (k == KNOB_RIGHT) change_custom_life(+1);
    }
    else if (lv_scr_act() == screen_player_counter_edit)
    {
        if (k == KNOB_LEFT)      change_multiplayer_counter_edit(-1);
        else if (k == KNOB_RIGHT) change_multiplayer_counter_edit(+1);
        refresh_multiplayer_counter_edit_ui();
    }
    else if (lv_scr_act() == screen_damage_log)
    {
        if (k == KNOB_LEFT)      damage_log_select_prev();
        else if (k == KNOB_RIGHT) damage_log_select_next();
    }
    else if (lv_scr_act() == screen_player_name)
    {
        if (k == KNOB_LEFT)      mru_select_prev();
        else if (k == KNOB_RIGHT) mru_select_next();
    }
}

void knob_change(knob_event_t k, int cont)
{
    uint8_t next_head;

    if (!knob_initialized)
    {
        last_knob_cont = cont;
        knob_initialized = true;
    }

    last_knob_cont = cont;

    next_head = (uint8_t)((knob_event_head + 1U) % KNOB_EVENT_QUEUE_SIZE);
    if (next_head == knob_event_tail) {
        knob_event_tail = (uint8_t)((knob_event_tail + 1U) % KNOB_EVENT_QUEUE_SIZE);
    }

    knob_event_queue[knob_event_head].event = k;
    knob_event_queue[knob_event_head].cont = cont;
    knob_event_head = next_head;
}

void knob_process_pending(void)
{
    uint8_t processed = 0;

    if (swipe_up_pending) {
        swipe_up_pending = false;
        open_menu_for_screen(lv_scr_act());
    }

    if (swipe_down_pending) {
        lv_obj_t *cur;

        swipe_down_pending = false;
        cur = lv_scr_act();

        if (is_player_screen(cur))
            open_menu_for_screen(cur);
        else
            handle_back_navigation(cur);
    }

    if (swipe_left_pending) {
        lv_obj_t *cur;

        swipe_left_pending = false;
        cur = lv_scr_act();

        if (is_player_screen(cur))
            open_menu_for_screen(cur);
        else
            handle_back_navigation(cur);
    }

    if (swipe_right_pending) {
        swipe_right_pending = false;
        open_menu_for_screen(lv_scr_act());
    }

    while (knob_event_tail != knob_event_head && processed < 8U) {
        knob_event_t event = knob_event_queue[knob_event_tail].event;
        knob_event_tail = (uint8_t)((knob_event_tail + 1U) % KNOB_EVENT_QUEUE_SIZE);
        handle_knob_event(event);
        processed++;
    }
}
