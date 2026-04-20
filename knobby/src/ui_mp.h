#ifndef _UI_MP_H
#define _UI_MP_H

#include "types.h"

// ---------- screens ----------
extern lv_obj_t *screen_multiplayer;

// ---------- functions ----------
void build_multiplayer_screen(void);
void rebuild_multiplayer_layout(int track);

void refresh_multiplayer_ui(void);

void open_multiplayer_screen(void);
void select_kick_timer(void);

#endif // _UI_MP_H
