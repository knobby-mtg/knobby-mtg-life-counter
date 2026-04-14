/*
 * Simulator lv_conf.h wrapper.
 * Includes the real knobby/lv_conf.h then overrides the few values
 * that differ for desktop simulation. Stays in sync automatically.
 */

#include "../knobby/lv_conf.h"

/* More memory on desktop — the 96KB pool can be tight for all screens */
#undef LV_MEM_SIZE
#define LV_MEM_SIZE (512U * 1024U)

/* No SPI byte swap needed on desktop — keeps lv_color_t fields in native order */
#undef LV_COLOR_16_SWAP
#define LV_COLOR_16_SWAP 0

/* Use POSIX clock instead of Arduino millis() */
#undef LV_TICK_CUSTOM_INCLUDE
#define LV_TICK_CUSTOM_INCLUDE "sim_stubs.h"

#undef LV_TICK_CUSTOM_SYS_TIME_EXPR
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (sim_millis())

/* abort() so desktop crashes give stack traces */
#undef LV_ASSERT_HANDLER
#define LV_ASSERT_HANDLER abort();
