#ifndef _PINCFG_H_
#define _PINCFG_H_

#include "board_detect.h"

/* Compatibility macros — all pin references resolve through the
 * runtime-detected board pointer set by board_detect(). */

#define TFT_BLK               (board->tft_blk)
#define TFT_RST               (board->tft_rst)
#define TFT_CS                (board->tft_cs)
#define TFT_SCK               (board->tft_sck)
#define TFT_SDA0              (board->tft_sda0)
#define TFT_SDA1              (board->tft_sda1)
#define TFT_SDA2              (board->tft_sda2)
#define TFT_SDA3              (board->tft_sda3)

#define TOUCH_PIN_NUM_I2C_SCL (board->touch_scl)
#define TOUCH_PIN_NUM_I2C_SDA (board->touch_sda)
#define TOUCH_PIN_NUM_INT     (board->touch_int)
#define TOUCH_PIN_NUM_RST     (board->touch_rst)

#define ROTARY_ENC_PIN_A      (board->enc_a)
#define ROTARY_ENC_PIN_B      (board->enc_b)

#define BATTERY_ADC_PIN_NUM   (board->bat_adc)

#define BTN_PIN               (board->btn)

#endif /* _PINCFG_H_ */
