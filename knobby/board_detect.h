#ifndef _BOARD_DETECT_H
#define _BOARD_DETECT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name;
    /* TFT QSPI */
    int tft_blk;
    int tft_rst;
    int tft_cs;
    int tft_sck;
    int tft_sda0;
    int tft_sda1;
    int tft_sda2;
    int tft_sda3;
    /* Touch I2C */
    int touch_scl;
    int touch_sda;
    int touch_int;
    int touch_rst;
    /* Rotary encoder */
    int enc_a;
    int enc_b;
    /* Battery ADC */
    int bat_adc;
    /* Button */
    int btn;
    /* Display orientation */
    bool mirror_x;
    bool mirror_y;
} board_pins_t;

/* Global pointer to detected board config. Set by board_detect(),
 * must be called before any pin-dependent init. */
extern const board_pins_t *board;

/* Known board configurations */
extern const board_pins_t board_k518;
extern const board_pins_t board_k718;

/* Probe I2C touch controllers to identify which board we're on.
 * Falls back to K518 if detection fails. */
void board_detect(void);

#ifdef __cplusplus
}
#endif

#endif /* _BOARD_DETECT_H */
