#include "knob_timer.h"

// Forward declaration
extern void refresh_turn_ui(void);
extern void back_to_main(void);

// ---------- state ----------
bool turn_timer_enabled = false;
bool turn_indicator_visible = true;
bool turn_ui_visible = false;
uint32_t turn_elapsed_ms = 0;
uint32_t turn_started_ms = 0;
int turn_number = 0;

static uint8_t turn_blink_steps_remaining = 0;

static lv_timer_t *turn_timer = NULL;
static lv_timer_t *turn_blink_timer = NULL;

// ---------- functions ----------
uint32_t get_turn_elapsed_ms(void)
{
    uint32_t elapsed = turn_elapsed_ms;

    if (turn_timer_enabled) {
        elapsed += lv_tick_elaps(turn_started_ms);
    }

    return elapsed;
}

void turn_timer_start_fresh(void)
{
    turn_number = 1;
    turn_elapsed_ms = 0;
    turn_started_ms = lv_tick_get();
    turn_timer_enabled = true;
    turn_indicator_visible = true;
    turn_ui_visible = true;
    turn_blink_steps_remaining = 10;

    if (turn_blink_timer != NULL) {
        lv_timer_resume(turn_blink_timer);
    }

    refresh_turn_ui();
}

void turn_timer_reset(void)
{
    turn_timer_enabled = false;
    turn_elapsed_ms = 0;
    turn_started_ms = 0;
    turn_number = 0;
    turn_indicator_visible = true;
    turn_ui_visible = false;
    turn_blink_steps_remaining = 0;

    if (turn_blink_timer != NULL) {
        lv_timer_pause(turn_blink_timer);
    }

    refresh_turn_ui();
}

// ---------- timer callbacks ----------
static void turn_timer_tick_cb(lv_timer_t *timer)
{
    (void)timer;
    refresh_turn_ui();
}

static void turn_blink_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (turn_blink_steps_remaining == 0) {
        turn_indicator_visible = true;
        if (turn_blink_timer != NULL) {
            lv_timer_pause(turn_blink_timer);
        }
        refresh_turn_ui();
        return;
    }

    turn_indicator_visible = !turn_indicator_visible;
    turn_blink_steps_remaining--;
    refresh_turn_ui();
}

// ---------- event callbacks ----------
void event_tool_timer(lv_event_t *e)
{
    (void)e;
    turn_timer_start_fresh();
    back_to_main();
}

void event_turn_tap(lv_event_t *e)
{
    (void)e;

    if (turn_number <= 0) {
        turn_number = 1;
        turn_elapsed_ms = 0;
    } else {
        turn_elapsed_ms = get_turn_elapsed_ms();
        turn_number++;
    }

    turn_started_ms = lv_tick_get();
    turn_timer_enabled = true;
    refresh_turn_ui();
}

// ---------- init ----------
void knob_timer_init(void)
{
    turn_timer = lv_timer_create(turn_timer_tick_cb, 1000, NULL);
    if (turn_timer != NULL) {
        lv_timer_ready(turn_timer);
    }

    turn_blink_timer = lv_timer_create(turn_blink_timer_cb, 500, NULL);
    if (turn_blink_timer != NULL) {
        lv_timer_pause(turn_blink_timer);
    }
}
