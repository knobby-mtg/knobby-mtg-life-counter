#ifndef _KNOB_TYPES_H
#define _KNOB_TYPES_H

#include "knob.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// ---------- constants ----------
#define MAX_ENEMY_COUNT 3
#define MAX_PLAYERS 4
#define LIFE_MIN -999
#define LIFE_MAX 999
#define COUNTER_MIN 0
#define COUNTER_MAX 9999
#define DEFAULT_LIFE_TOTAL 40
#define DEFAULT_BRIGHTNESS_PERCENT 30
#define INTRO_CHAR_COUNT 7
#define MULTIPLAYER_COUNT 4
#define KNOB_EVENT_QUEUE_SIZE 32

// ---------- color modes ----------
#define COLOR_MODE_PLAYER 0
#define COLOR_MODE_LIFE   1
#define COLOR_MODE_COUNT  2

// ---------- orientation modes ----------
#define ORIENTATION_MODE_ABSOLUTE 0
#define ORIENTATION_MODE_CENTRIC  1
#define ORIENTATION_MODE_TABLETOP 2
#define ORIENTATION_MODE_COUNT    3

// ---------- auto-dim timeout options ----------
#define AUTO_DIM_OFF  0
#define AUTO_DIM_15S  1
#define AUTO_DIM_30S  2
#define AUTO_DIM_60S  3
#define AUTO_DIM_COUNT 4

static const uint32_t auto_dim_ms[] = {0, 15000, 30000, 60000};

// ---------- deselect timeout options ----------
#define DESELECT_NEVER 0
#define DESELECT_5S    1
#define DESELECT_15S   2
#define DESELECT_30S   3
#define DESELECT_COUNT 4

static const int deselect_ms[] = {0, 5000, 15000, 30000};

// ---------- types ----------
typedef struct {
    const char *name;
    int damage;
} enemy_state_t;

typedef struct {
    knob_event_t event;
    int cont;
} knob_input_event_t;

typedef struct {
    const char *label;
    lv_event_cb_t cb;
    bool enabled;
    lv_event_code_t event;
} quad_item_t;

// ---------- 7-segment map ----------
static const uint8_t seg_map[10][7] = {
    {1,1,1,0,1,1,1}, // 0
    {0,0,1,0,0,1,0}, // 1
    {1,0,1,1,1,0,1}, // 2
    {1,0,1,1,0,1,1}, // 3
    {0,1,1,1,0,1,0}, // 4
    {1,1,0,1,0,1,1}, // 5
    {1,1,0,1,1,1,1}, // 6
    {1,0,1,0,0,1,0}, // 7
    {1,1,1,1,1,1,1}, // 8
    {1,1,1,1,0,1,1}  // 9
};

// ---------- utility functions ----------
static inline int clamp_life(int value)
{
    if (value < LIFE_MIN) return LIFE_MIN;
    if (value > LIFE_MAX) return LIFE_MAX;
    return value;
}

static inline int clamp_brightness(int value)
{
    if (value < 1) return 1;
    if (value > 100) return 100;
    return value;
}

static inline int clamp_counter(int value)
{
    if (value < COUNTER_MIN) return COUNTER_MIN;
    if (value > COUNTER_MAX) return COUNTER_MAX;
    return value;
}

static inline int get_arc_display_value(int value, int max_life)
{
    if (value < 0) return 0;
    if (value > max_life) return max_life;
    return value;
}

// ---------- life color tiers ----------
#define LIFE_TIER_RED    0
#define LIFE_TIER_YELLOW 1
#define LIFE_TIER_GREEN  2
#define LIFE_TIER_PURPLE 3
#define LIFE_TIER_COUNT  4

#define LIFE_VIB_DIM  0
#define LIFE_VIB_MID  1
#define LIFE_VIB_VIV  2
#define LIFE_VIB_COUNT 3

static const uint32_t life_color_table[LIFE_TIER_COUNT][LIFE_VIB_COUNT] = {
    /* dim        mid        vivid */
    {0x4D1C1C, 0xF44336, 0xFF0000},  /* red    */
    {0x4D4D00, 0xFFEB3B, 0xFFFF00},  /* yellow */
    {0x1A4D1A, 0x4CAF50, 0x00FF00},  /* green  */
    {0x2E004D, 0x7B1FA2, 0xAA00FF},  /* purple */
};

static inline int get_life_tier(int value, int max_life)
{
    if (value > max_life)          return LIFE_TIER_PURPLE;
    if (value >= max_life * 3 / 4) return LIFE_TIER_GREEN;
    if (value >= max_life / 4)     return LIFE_TIER_YELLOW;
    return LIFE_TIER_RED;
}

static inline lv_color_t get_life_color(int value, int max_life)
{
    return lv_color_hex(life_color_table[get_life_tier(value, max_life)][LIFE_VIB_MID]);
}

static inline lv_color_t get_life_color_vib(int tier, int vibrancy)
{
    return lv_color_hex(life_color_table[tier][vibrancy]);
}

static inline bool color_is_light(lv_color_t c)
{
    uint32_t c32 = lv_color_to32(c);
    int r = (c32 >> 16) & 0xFF;
    int g = (c32 >> 8) & 0xFF;
    int b = c32 & 0xFF;
    int lum = r * 299 + g * 587 + b * 114;
    return lum > 128000;
}

// ---------- LVGL widget helpers ----------
static inline lv_obj_t *make_button(lv_obj_t *parent, const char *txt,
                                     lv_coord_t w, lv_coord_t h,
                                     lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, txt);
    lv_obj_center(label);
    return btn;
}

static inline lv_obj_t *make_plain_box(lv_obj_t *parent,
                                        lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, w, h);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return obj;
}

static inline lv_obj_t *make_seg(lv_obj_t *parent,
                                  lv_coord_t x, lv_coord_t y,
                                  lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_radius(obj, 2, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static inline void load_screen_if_needed(lv_obj_t *screen)
{
    if (screen != NULL && lv_scr_act() != screen) {
        lv_scr_load(screen);
    }
}

#endif // _KNOB_TYPES_H
