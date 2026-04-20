#include <SDL2/SDL.h>
#include <lvgl.h>
#include "sim_stubs.h"
#include "board_detect.h"
#include "knob.h"
#include "hw.h"
#include "storage.h"
#include "game.h"
#include "ui_1p.h"

#include <stdio.h>
#include <stdbool.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define millis sim_millis
#define SCREEN_W 360
#define SCREEN_H 360

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static bool quit = false;

static lv_color_t framebuffer[SCREEN_W * SCREEN_H];
static lv_color_t draw_buf_data[SCREEN_W * 72];
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;

static uint32_t last_tick = 0;

static void sim_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    int x, y;
    for (y = area->y1; y <= area->y2; y++) {
        for (x = area->x1; x <= area->x2; x++) {
            framebuffer[y * SCREEN_W + x] = *color_p++;
        }
    }
    lv_disp_flush_ready(drv);
}

static void update_sdl_display(void)
{
    void *pixels;
    int pitch;
    const int cx = SCREEN_W / 2;
    const int cy = SCREEN_H / 2;
    const int rr = (SCREEN_W / 2) * (SCREEN_W / 2);
    const uint32_t bezel = 0xFF505050;
    SDL_LockTexture(texture, NULL, &pixels, &pitch);

    uint32_t *p = (uint32_t *)pixels;
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            int i = y * SCREEN_W + x;
            int dx = x - cx;
            int dy = y - cy;
            if (dx * dx + dy * dy > rr) {
                p[i] = bezel;
            } else {
                lv_color_t c = framebuffer[i];
                uint8_t r = (uint8_t)((c.ch.red * 255) / 31);
                uint8_t g = (uint8_t)((c.ch.green * 255) / 63);
                uint8_t b = (uint8_t)((c.ch.blue * 255) / 31);
                p[i] = (0xFF << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }

    SDL_UnlockTexture(texture);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

static void handle_sdl_events(void);
static void main_loop(void);
void sdl_mouse_read(lv_indev_drv_t * drv, lv_indev_data_t * data);

static void main_loop(void)
{
    uint32_t current_tick = SDL_GetTicks();
    uint32_t diff = current_tick - last_tick;

    handle_sdl_events();

    if (diff > 0) {
        sim_tick_advance(diff); 
        last_tick = current_tick;
    }
    
    lv_timer_handler();
    knob_process_pending();
    update_sdl_display();

#ifndef __EMSCRIPTEN__
    SDL_Delay(5);
#endif
}

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Knobby Simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);

    board_detect();
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, draw_buf_data, NULL, SCREEN_W * 72);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_W;
    disp_drv.ver_res = SCREEN_H;
    disp_drv.flush_cb = sim_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Register mouse as touchpad
    static lv_indev_drv_t indev_drv_tp;
    lv_indev_drv_init(&indev_drv_tp);
    indev_drv_tp.type = LV_INDEV_TYPE_POINTER;
    indev_drv_tp.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&indev_drv_tp);

    knob_gui();

    // Set some defaults if needed
    nvs_set_players_to_track(4);
    back_to_main();

    last_tick = SDL_GetTicks();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while (!quit) {
        main_loop();
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif

    return 0;
}

static void handle_sdl_events(void)
{
    SDL_Event event;
    static int encoder_cont = 0;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            quit = true;
        } else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION) {
            // Handled by sdl_mouse_read
        } else if (event.type == SDL_MOUSEWHEEL) {
            if (event.wheel.y > 0) {
                encoder_cont++;
                knob_change(KNOB_RIGHT, encoder_cont);
            } else if (event.wheel.y < 0) {
                encoder_cont--;
                knob_change(KNOB_LEFT, encoder_cont);
            }
        } else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_RIGHT) {
                encoder_cont++;
                knob_change(KNOB_RIGHT, encoder_cont);
            } else if (event.key.keysym.sym == SDLK_LEFT) {
                encoder_cont--;
                knob_change(KNOB_LEFT, encoder_cont);
            } else if (event.key.keysym.sym == SDLK_UP) {
                knob_notify_swipe_up();
            } else if (event.key.keysym.sym == SDLK_DOWN) {
                knob_notify_swipe_down();
            } else if (event.key.keysym.sym == SDLK_l) {
                knob_notify_swipe_left();
            } else if (event.key.keysym.sym == SDLK_r) {
                knob_notify_swipe_right();
            } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                quit = true;
            }
        }
    }
}

void sdl_mouse_read(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    int x, y;
    uint32_t buttons = SDL_GetMouseState(&x, &y);
    
    data->point.x = (lv_coord_t)x;
    data->point.y = (lv_coord_t)y;
    data->state = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

