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
#define DEFAULT_LIFE_TOTAL 40
#define DEFAULT_BRIGHTNESS_PERCENT 30
#define KNOBBY_LETTER_COUNT 6
#define INTRO_CHAR_COUNT 7
#define MULTIPLAYER_COUNT 4
#define KNOB_EVENT_QUEUE_SIZE 32

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
    if (value < 5) return 5;
    if (value > 100) return 100;
    return value;
}

static inline int get_arc_display_value(int value, int max_life)
{
    if (value < 0) return 0;
    if (value > max_life) return max_life;
    return value;
}

static inline lv_color_t get_life_color(int value, int max_life)
{
    if (value > max_life)          return lv_palette_main(LV_PALETTE_PURPLE);
    if (value >= max_life * 3 / 4) return lv_palette_main(LV_PALETTE_GREEN);
    if (value >= max_life / 4)     return lv_palette_main(LV_PALETTE_YELLOW);
    return lv_palette_main(LV_PALETTE_RED);
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
