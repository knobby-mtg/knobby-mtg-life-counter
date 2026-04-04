#ifndef _KNOB_NVS_H
#define _KNOB_NVS_H

#include "knob_types.h"

void knob_nvs_init(void);
void settings_save(void);

int nvs_get_brightness(void);
void nvs_set_brightness(int value);
bool nvs_get_auto_dim(void);
void nvs_set_auto_dim(bool value);

int nvs_get_num_players(void);
void nvs_set_num_players(int value);
int nvs_get_players_to_track(void);
void nvs_set_players_to_track(int value);
int nvs_get_life_total(void);
void nvs_set_life_total(int value);

#endif // _KNOB_NVS_H
