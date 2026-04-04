#include "knob_nvs.h"
#include "nvs_flash.h"
#include "nvs.h"

// ---------- cached state ----------
static bool settings_dirty = false;
static int cached_brightness = DEFAULT_BRIGHTNESS_PERCENT;
static bool cached_auto_dim = false;
static int cached_num_players = 4;
static int cached_players_to_track = 1;
static int cached_life_total = DEFAULT_LIFE_TOTAL;

// ---------- init ----------
void knob_nvs_init(void)
{
    nvs_flash_init();

    nvs_handle_t handle;
    if (nvs_open("knobby", NVS_READONLY, &handle) == ESP_OK) {
        int8_t dim_val = 0;
        int8_t bri_val = DEFAULT_BRIGHTNESS_PERCENT;
        int8_t np_val = 4;
        int8_t pt_val = 1;
        int16_t lt_val = DEFAULT_LIFE_TOTAL;

        nvs_get_i8(handle, "auto_dim", &dim_val);
        nvs_get_i8(handle, "brightness", &bri_val);
        nvs_get_i8(handle, "num_players", &np_val);
        nvs_get_i8(handle, "track", &pt_val);
        nvs_get_i16(handle, "life_total", &lt_val);

        cached_auto_dim = (dim_val != 0);
        cached_brightness = clamp_brightness(bri_val);
        cached_num_players = (np_val < 1) ? 1 : (np_val > MAX_PLAYERS) ? MAX_PLAYERS : np_val;
        cached_players_to_track = (pt_val < 1) ? 1 : (pt_val > 4) ? 4 : pt_val;
        cached_life_total = (lt_val < 0) ? 0 : (lt_val > LIFE_MAX) ? LIFE_MAX : lt_val;

        nvs_close(handle);
    }
}

// ---------- getters ----------
int nvs_get_brightness(void)
{
    return cached_brightness;
}

bool nvs_get_auto_dim(void)
{
    return cached_auto_dim;
}

// ---------- setters ----------
void nvs_set_brightness(int value)
{
    cached_brightness = clamp_brightness(value);
    settings_dirty = true;
}

void nvs_set_auto_dim(bool value)
{
    cached_auto_dim = value;
    settings_dirty = true;
}

// ---------- game mode getters/setters ----------
int nvs_get_num_players(void)
{
    return cached_num_players;
}

void nvs_set_num_players(int value)
{
    cached_num_players = (value < 1) ? 1 : (value > MAX_PLAYERS) ? MAX_PLAYERS : value;
    settings_dirty = true;
}

int nvs_get_players_to_track(void)
{
    return cached_players_to_track;
}

void nvs_set_players_to_track(int value)
{
    cached_players_to_track = (value < 1) ? 1 : (value > 4) ? 4 : value;
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

// ---------- persist ----------
void settings_save(void)
{
    if (!settings_dirty) return;
    nvs_handle_t handle;
    if (nvs_open("knobby", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_i8(handle, "auto_dim", cached_auto_dim ? 1 : 0);
        nvs_set_i8(handle, "brightness", (int8_t)cached_brightness);
        nvs_set_i8(handle, "num_players", (int8_t)cached_num_players);
        nvs_set_i8(handle, "track", (int8_t)cached_players_to_track);
        nvs_set_i16(handle, "life_total", (int16_t)cached_life_total);
        nvs_commit(handle);
        nvs_close(handle);
        settings_dirty = false;
    }
}
