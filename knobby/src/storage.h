#ifndef _STORAGE_H
#define _STORAGE_H

#include "types.h"

void knob_nvs_init(void);
void settings_save(void);

int nvs_get_brightness(void);
void nvs_set_brightness(int value);
int nvs_get_auto_dim(void);
void nvs_set_auto_dim(int value);

int nvs_get_color_mode(void);
void nvs_set_color_mode(int value);
int nvs_get_deselect_timeout(void);
void nvs_set_deselect_timeout(int value);
int nvs_get_orientation(void);
void nvs_set_orientation(int value);

int nvs_get_num_players(void);
void nvs_set_num_players(int value);
int nvs_get_players_to_track(void);
void nvs_set_players_to_track(int value);
int nvs_get_life_total(void);
void nvs_set_life_total(int value);

int nvs_get_auto_eliminate(void);
void nvs_set_auto_eliminate(int value);

#define NAME_LIST_COUNT 10
#define NAME_LIST_LEN   16
void nvs_get_name_list(char (*out)[NAME_LIST_LEN]);
void nvs_set_name_list(const char (*list)[NAME_LIST_LEN]);

#endif // _STORAGE_H
