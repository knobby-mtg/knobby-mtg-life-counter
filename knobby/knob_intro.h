#ifndef _KNOB_INTRO_H
#define _KNOB_INTRO_H

#include "knob_types.h"

extern lv_obj_t *screen_intro;

void build_intro_screen(void);
void refresh_intro_ui(void);
void knob_intro_init(void);

#endif // _KNOB_INTRO_H
