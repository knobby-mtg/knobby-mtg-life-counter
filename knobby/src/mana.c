#include "mana.h"
#include <string.h>
#include <stdio.h>

// ---------- mana icon codepoints (from andrewgioia/mana font) ----------
#define MANA_ICON_WHITE     "\xEE\x98\x80"  /* U+E600 */
#define MANA_ICON_BLUE      "\xEE\x98\x81"  /* U+E601 */
#define MANA_ICON_BLACK     "\xEE\x98\x82"  /* U+E602 */
#define MANA_ICON_RED       "\xEE\x98\x83"  /* U+E603 */
#define MANA_ICON_GREEN     "\xEE\x98\x84"  /* U+E604 */
#define MANA_ICON_COLORLESS "\xEE\xA4\x84"  /* U+E904 */

// ---------- color tables ----------
// Official mana colors from mana.andrewgioia.com, dimmed for box backgrounds
static const uint32_t mana_bg_colors[MANA_COLOR_COUNT] = {
    0x3D3D30,  /* W: warm dark */
    0x0D2A4D,  /* U: dark blue */
    0x1A0A2A,  /* B: dark purple */
    0x4D1A1A,  /* R: dark red */
    0x0A3D1A,  /* G: dark green */
    0x2A2A2A,  /* C: dark gray */
};

static const uint32_t mana_active_colors[MANA_COLOR_COUNT] = {
    0xFDFBCE,  /* W */
    0xBCDAF7,  /* U */
    0xA7999E,  /* B */
    0xF19B79,  /* R */
    0x9FCBA6,  /* G */
    0xD0C6BB,  /* C */
};

static const char *mana_icons[MANA_COLOR_COUNT] = {
    MANA_ICON_WHITE, MANA_ICON_BLUE, MANA_ICON_BLACK,
    MANA_ICON_RED, MANA_ICON_GREEN, MANA_ICON_COLORLESS,
};

// ---------- state ----------
lv_obj_t *screen_mana = NULL;
int mana_values[MANA_COLOR_COUNT] = {0};
static int mana_selected = -1;  /* -1 = none selected */
static int mana_pending_delta = 0;
static bool mana_preview_active = false;
static lv_timer_t *mana_preview_timer = NULL;
#define MANA_PREVIEW_MS 3000

// ---------- widget references ----------
static lv_obj_t *mana_boxes[MANA_COLOR_COUNT] = {NULL};
static lv_obj_t *mana_value_labels[MANA_COLOR_COUNT] = {NULL};

// ---------- layout constants ----------
#define BOX_W       90
#define BOX_H       70
#define BOX_GAP_X   6
#define BOX_GAP_Y   6
#define GRID_COLS   3
#define GRID_X      39   /* (360 - (3*90 + 2*6)) / 2 */
#define GRID_Y      82
#define MANA_MAX    999

LV_FONT_DECLARE(mana_counter_icons_16);

// ---------- helpers ----------
static inline int clamp_mana(int v)
{
    return (v < 0) ? 0 : (v > MANA_MAX) ? MANA_MAX : v;
}

// ---------- preview commit ----------
static void mana_preview_commit(lv_timer_t *timer)
{
    (void)timer;
    if (mana_preview_active && mana_selected >= 0) {
        mana_values[mana_selected] = clamp_mana(mana_values[mana_selected] + mana_pending_delta);
    }
    mana_pending_delta = 0;
    mana_preview_active = false;
    if (mana_preview_timer != NULL)
        lv_timer_pause(mana_preview_timer);
    refresh_mana_ui();
}

// ---------- event handlers ----------
static void event_mana_box_tap(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    /* commit any pending preview before switching */
    if (mana_preview_active)
        mana_preview_commit(NULL);
    if (mana_selected == idx) {
        mana_selected = -1;  /* deselect */
    } else {
        mana_selected = idx;
    }
    refresh_mana_ui();
}

static void event_mana_clear(lv_event_t *e)
{
    (void)e;
    mana_clear_all();
}

// ---------- public API ----------
void change_mana_value(int delta)
{
    int base;
    if (mana_selected < 0) return;

    base = mana_values[mana_selected];
    mana_pending_delta += delta;
    /* clamp the total (base + delta) so we don't exceed bounds */
    mana_pending_delta = clamp_mana(base + mana_pending_delta) - base;
    mana_preview_active = (mana_pending_delta != 0);

    if (mana_preview_timer != NULL)
        lv_timer_reset(mana_preview_timer);

    if (!mana_preview_active && mana_preview_timer != NULL) {
        lv_timer_pause(mana_preview_timer);
    } else if (mana_preview_timer != NULL) {
        lv_timer_resume(mana_preview_timer);
    }

    refresh_mana_ui();
}

void mana_set_selected(int idx)
{
    mana_selected = (idx < -1) ? -1 : (idx >= MANA_COLOR_COUNT) ? -1 : idx;
}

void mana_clear_all(void)
{
    memset(mana_values, 0, sizeof(mana_values));
    mana_selected = -1;
    mana_pending_delta = 0;
    mana_preview_active = false;
    if (mana_preview_timer != NULL)
        lv_timer_pause(mana_preview_timer);
    refresh_mana_ui();
}

void refresh_mana_ui(void)
{
    char buf[8];
    int i;

    if (screen_mana == NULL) return;

    for (i = 0; i < MANA_COLOR_COUNT; i++) {
        if (mana_boxes[i] == NULL) continue;

        /* show pending delta for selected box, committed value for others */
        if (i == mana_selected && mana_preview_active) {
            snprintf(buf, sizeof(buf), "%+d", mana_pending_delta);
        } else {
            snprintf(buf, sizeof(buf), "%d", mana_values[i]);
        }
        lv_label_set_text(mana_value_labels[i], buf);

        /* highlight selected box */
        if (i == mana_selected) {
            lv_obj_set_style_border_color(mana_boxes[i],
                lv_color_hex(mana_active_colors[i]), 0);
            lv_obj_set_style_border_width(mana_boxes[i], 3, 0);
        } else {
            lv_obj_set_style_border_width(mana_boxes[i], 0, 0);
        }
    }
}

void mana_discard_preview(void)
{
    mana_pending_delta = 0;
    mana_preview_active = false;
    if (mana_preview_timer != NULL)
        lv_timer_pause(mana_preview_timer);
}

void open_mana_screen(void)
{
    mana_discard_preview();
    refresh_mana_ui();
    load_screen_if_needed(screen_mana);
}

void event_tool_mana(lv_event_t *e)
{
    (void)e;
    open_mana_screen();
}

// ---------- build ----------
void build_mana_screen(void)
{
    int i, col, row, x, y;
    lv_obj_t *label;

    screen_mana = lv_obj_create(NULL);
    lv_obj_set_size(screen_mana, 360, 360);
    lv_obj_set_style_bg_color(screen_mana, lv_color_black(), 0);
    lv_obj_set_style_border_width(screen_mana, 0, 0);
    lv_obj_set_scrollbar_mode(screen_mana, LV_SCROLLBAR_MODE_OFF);

    /* title */
    label = lv_label_create(screen_mana);
    lv_label_set_text(label, "Mana Pool");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_22, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 36);

    /* 3x2 grid of mana boxes */
    for (i = 0; i < MANA_COLOR_COUNT; i++) {
        col = i % GRID_COLS;
        row = i / GRID_COLS;
        x = GRID_X + col * (BOX_W + BOX_GAP_X);
        y = GRID_Y + row * (BOX_H + BOX_GAP_Y);

        lv_obj_t *box = lv_obj_create(screen_mana);
        lv_obj_set_size(box, BOX_W, BOX_H);
        lv_obj_set_pos(box, x, y);
        lv_obj_set_style_bg_color(box, lv_color_hex(mana_bg_colors[i]), 0);
        lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(box, 8, 0);
        lv_obj_set_style_border_width(box, 0, 0);
        lv_obj_set_style_pad_all(box, 0, 0);
        lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_OFF);
        lv_obj_add_flag(box, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(box, event_mana_box_tap, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);

        /* mana icon */
        lv_obj_t *icon = lv_label_create(box);
        lv_label_set_text(icon, mana_icons[i]);
        lv_obj_set_style_text_font(icon, &mana_counter_icons_16, 0);
        lv_obj_set_style_text_color(icon,
            lv_color_hex(mana_active_colors[i]), 0);
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 6);

        /* value label */
        lv_obj_t *val = lv_label_create(box);
        lv_label_set_text(val, "0");
        lv_obj_set_style_text_font(val, &lv_font_montserrat_32, 0);
        lv_obj_set_style_text_color(val, lv_color_white(), 0);
        lv_obj_align(val, LV_ALIGN_BOTTOM_MID, 0, -4);

        mana_boxes[i] = box;
        mana_value_labels[i] = val;
    }

    /* clear all button (long-press) */
    {
        lv_obj_t *btn = lv_btn_create(screen_mana);
        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, 160, 46);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -46);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_add_event_cb(btn, event_mana_clear, LV_EVENT_LONG_PRESSED, NULL);

        label = lv_label_create(btn);
        lv_label_set_text(label, "Clear All\n(Long Press)");
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }

    /* preview commit timer (3s, starts paused) */
    mana_preview_timer = lv_timer_create(mana_preview_commit, MANA_PREVIEW_MS, NULL);
    if (mana_preview_timer != NULL)
        lv_timer_pause(mana_preview_timer);
}
