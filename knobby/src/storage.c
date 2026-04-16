#include "storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

// ---------- cached state ----------
static bool settings_dirty = false;
static int cached_brightness = DEFAULT_BRIGHTNESS_PERCENT;
static int cached_auto_dim = AUTO_DIM_OFF;
static int cached_color_mode = 0;
static int cached_deselect_timeout = 0; /* index: 0=never, 1=5s, 2=15s, 3=30s */
static int cached_orientation = ORIENTATION_MODE_ABSOLUTE;
static int cached_num_players = 4;
static int cached_players_to_track = 1;
static int cached_life_total = DEFAULT_LIFE_TOTAL;
static int cached_auto_eliminate = 1; /* 1=ON (default), 0=OFF */
static char cached_name_list[NAME_LIST_COUNT][NAME_LIST_LEN];

// ---------- init ----------
void knob_nvs_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    nvs_handle_t handle;
    if (nvs_open("knobby", NVS_READONLY, &handle) == ESP_OK) {
        int8_t dim_val = 0;
        int8_t bri_val = DEFAULT_BRIGHTNESS_PERCENT;
        int8_t lc_val = 0;
        int8_t dt_val = 0;
        int8_t rot_val = 0;
        int8_t np_val = 4;
        int8_t pt_val = 1;
        int16_t lt_val = DEFAULT_LIFE_TOTAL;

        nvs_get_i8(handle, "auto_dim", &dim_val);
        nvs_get_i8(handle, "brightness", &bri_val);
        nvs_get_i8(handle, "color_mode", &lc_val);
        nvs_get_i8(handle, "desel_time", &dt_val);
        nvs_get_i8(handle, "rotation", &rot_val);
        nvs_get_i8(handle, "num_players", &np_val);
        nvs_get_i8(handle, "track", &pt_val);
        nvs_get_i16(handle, "life_total", &lt_val);

        cached_auto_dim = (dim_val < 0) ? AUTO_DIM_OFF : (dim_val >= AUTO_DIM_COUNT) ? AUTO_DIM_OFF : dim_val;
        cached_color_mode = (lc_val < 0) ? COLOR_MODE_PLAYER : (lc_val >= COLOR_MODE_COUNT) ? COLOR_MODE_PLAYER : lc_val;
        cached_deselect_timeout = (dt_val < 0) ? 0 : (dt_val > 3) ? 3 : dt_val;
        cached_orientation = (rot_val < 0) ? ORIENTATION_MODE_ABSOLUTE
                : (rot_val >= ORIENTATION_MODE_COUNT) ? ORIENTATION_MODE_ABSOLUTE
                                   : rot_val;
        cached_brightness = clamp_brightness(bri_val);
        cached_num_players = (np_val < 1) ? 1 : (np_val > MAX_GAME_PLAYERS) ? MAX_GAME_PLAYERS : np_val;
        cached_players_to_track = (pt_val < 1) ? 1 : (pt_val > MAX_DISPLAY_PLAYERS) ? MAX_DISPLAY_PLAYERS : pt_val;
        cached_life_total = (lt_val < 0) ? 0 : (lt_val > LIFE_MAX) ? LIFE_MAX : lt_val;

        int8_t ae_val = 1;
        nvs_get_i8(handle, "auto_elim", &ae_val);
        cached_auto_eliminate = (ae_val != 0) ? 1 : 0;

        size_t nl_size = sizeof(cached_name_list);
        nvs_get_blob(handle, "name_list", cached_name_list, &nl_size);
        for (int i = 0; i < NAME_LIST_COUNT; i++)
            cached_name_list[i][NAME_LIST_LEN - 1] = '\0';

        nvs_close(handle);
    }
}

// ---------- getters ----------
int nvs_get_brightness(void)
{
    return cached_brightness;
}

int nvs_get_auto_dim(void)
{
    return cached_auto_dim;
}

// ---------- setters ----------
void nvs_set_brightness(int value)
{
    cached_brightness = clamp_brightness(value);
    settings_dirty = true;
}

void nvs_set_auto_dim(int value)
{
    cached_auto_dim = (value < 0) ? AUTO_DIM_OFF : (value >= AUTO_DIM_COUNT) ? AUTO_DIM_OFF : value;
    settings_dirty = true;
}

int nvs_get_color_mode(void)
{
    return cached_color_mode;
}

void nvs_set_color_mode(int value)
{
    cached_color_mode = (value < 0) ? COLOR_MODE_PLAYER : (value >= COLOR_MODE_COUNT) ? COLOR_MODE_PLAYER : value;
    settings_dirty = true;
}

int nvs_get_deselect_timeout(void)
{
    return cached_deselect_timeout;
}

void nvs_set_deselect_timeout(int value)
{
    cached_deselect_timeout = (value < 0) ? 0 : (value > 3) ? 3 : value;
    settings_dirty = true;
}

int nvs_get_orientation(void)
{
    return cached_orientation;
}

void nvs_set_orientation(int value)
{
    cached_orientation = (value < 0) ? ORIENTATION_MODE_ABSOLUTE
                                      : (value >= ORIENTATION_MODE_COUNT) ? ORIENTATION_MODE_ABSOLUTE
                                                                       : value;
    settings_dirty = true;
}

// ---------- game mode getters/setters ----------
int nvs_get_num_players(void)
{
    return cached_num_players;
}

void nvs_set_num_players(int value)
{
    cached_num_players = (value < 1) ? 1 : (value > MAX_GAME_PLAYERS) ? MAX_GAME_PLAYERS : value;
    settings_dirty = true;
}

int nvs_get_players_to_track(void)
{
    return cached_players_to_track;
}

void nvs_set_players_to_track(int value)
{
    cached_players_to_track = (value < 1) ? 1 : (value > MAX_DISPLAY_PLAYERS) ? MAX_DISPLAY_PLAYERS : value;
    settings_dirty = true;
}

int nvs_get_life_total(void)
{
    return cached_life_total;
}

void nvs_set_life_total(int value)
{
    cached_life_total = (value < 0) ? 0 : (value > LIFE_MAX) ? LIFE_MAX : value;
    settings_dirty = true;
}

// ---------- auto-eliminate ----------
int nvs_get_auto_eliminate(void)
{
    return cached_auto_eliminate;
}

void nvs_set_auto_eliminate(int value)
{
    cached_auto_eliminate = (value != 0) ? 1 : 0;
    settings_dirty = true;
}

// ---------- name list ----------
void nvs_get_name_list(char (*out)[NAME_LIST_LEN])
{
    memcpy(out, cached_name_list, sizeof(cached_name_list));
}

void nvs_set_name_list(const char (*list)[NAME_LIST_LEN])
{
    memcpy(cached_name_list, list, sizeof(cached_name_list));
    settings_dirty = true;
}

// ---------- persist ----------
void settings_save(void)
{
    if (!settings_dirty) return;
    nvs_handle_t handle;
    if (nvs_open("knobby", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_i8(handle, "auto_dim", (int8_t)cached_auto_dim);
        nvs_set_i8(handle, "brightness", (int8_t)cached_brightness);
        nvs_set_i8(handle, "color_mode", (int8_t)cached_color_mode);
        nvs_set_i8(handle, "desel_time", (int8_t)cached_deselect_timeout);
        nvs_set_i8(handle, "rotation", (int8_t)cached_orientation);
        nvs_set_i8(handle, "num_players", (int8_t)cached_num_players);
        nvs_set_i8(handle, "track", (int8_t)cached_players_to_track);
        nvs_set_i16(handle, "life_total", (int16_t)cached_life_total);
        nvs_set_i8(handle, "auto_elim", (int8_t)cached_auto_eliminate);
        nvs_set_blob(handle, "name_list", cached_name_list, sizeof(cached_name_list));
        nvs_commit(handle);
        nvs_close(handle);
        settings_dirty = false;
    }
}
