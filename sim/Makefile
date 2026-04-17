# Knobby Screenshot Simulator
# Requires: gcc, LVGL installed via arduino-cli (see CONTRIBUTING.md)

# Include configuration for paths
-include ../config.mk

# Default values if not passed from config.sh
LLVM_BIN  ?= 
LVGL_PATH ?= $(HOME)/Arduino/libraries/lvgl
PYTHON    ?= python3

KNOBBY    := ../knobby
BUILD     := build
BIN       := knobby_sim

ifeq ($(OS),Windows_NT)
    SDL2_ROOT   := SDL2/x86_64-w64-mingw32
    BIN_GUI     := knobby_sim_gui.exe
    CC          ?= $(LLVM_BIN)/clang.exe
    SDL_CFLAGS  := -I$(SDL2_ROOT)/include
    SDL_LDFLAGS := -L$(SDL2_ROOT)/lib -lSDL2main -lSDL2
else
    BIN_GUI     := knobby_sim_gui
    CC          ?= gcc
    SDL_CFLAGS  := $(shell pkg-config --cflags sdl2 2>/dev/null)
    SDL_LDFLAGS := $(shell pkg-config --libs sdl2 2>/dev/null || echo "-lSDL2")
endif

CFLAGS    := -Wall -Wno-unused-function \
             -DLV_CONF_INCLUDE_SIMPLE -DSIMULATOR=1 \
             -I. -Iesp_stubs -I$(LVGL_PATH) -I$(LVGL_PATH)/src -I$(KNOBBY) -I$(KNOBBY)/src \
             -O2 -g

LDFLAGS   := -lm

# SDL2 flags are only used for the GUI build, not the headless screenshot simulator
GUI_CFLAGS  := $(CFLAGS) $(SDL_CFLAGS)
GUI_LDFLAGS := $(SDL_LDFLAGS) $(LDFLAGS)

# ---- Source files ----

# Top-level knobby files
KNOBBY_TOP_SRCS := $(KNOBBY)/knob.c

# Source files in src/
KNOBBY_SRC_SRCS := $(addprefix $(KNOBBY)/src/, \
	ui_1p.c ui_mp.c ui_player_menu.c settings.c \
	intro.c timer.c game.c damage_log.c \
	game_mode.c rename.c dice.c hw.c storage.c \
	mana.c wireless_manager.c ui_wireless.c)

# Sim files
SIM_SRCS := sim_main.c sim_stubs.c

# LVGL sources (recursive wildcard)
rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
LVGL_SRCS := $(call rwildcard,$(LVGL_PATH)/src/,*.c)

FONT_SRCS := $(wildcard $(KNOBBY)/src/fonts/*.c)

ALL_SRCS := $(SIM_SRCS) $(KNOBBY_TOP_SRCS) $(KNOBBY_SRC_SRCS) $(FONT_SRCS) $(LVGL_SRCS)
ALL_SRCS_GUI := sim_sdl2.c sim_stubs.c $(KNOBBY_TOP_SRCS) $(KNOBBY_SRC_SRCS) $(FONT_SRCS) $(LVGL_SRCS)

# Object files — flatten into build/ using path-mangled names
obj_name = $(BUILD)/$(subst :,_COL_,$(subst /,__,$(subst ..,UP,$(1:.c=.o))))
ALL_OBJS := $(foreach src,$(ALL_SRCS),$(call obj_name,$(src)))
ALL_OBJS_GUI := $(foreach src,$(ALL_SRCS_GUI),$(call obj_name,$(src)))

# ---- Targets ----

.PHONY: screenshot generate-matrix clean gui web sim-gui

$(BIN): $(ALL_OBJS)
	$(CC) $(ALL_OBJS) -o $@ $(LDFLAGS)

sim-gui: $(BIN_GUI)
	./$(BIN_GUI)

gui: $(BIN_GUI)
	./$(BIN_GUI)

$(BIN_GUI): $(ALL_OBJS_GUI)
	$(CC) $(ALL_OBJS_GUI) -o $@ $(GUI_LDFLAGS)

web:
	$(PYTHON) gen_web_sources.py $(LVGL_PATH) $(KNOBBY)
	$(EMCC) $(CFLAGS) -o knobby_web.js @web_sources.txt \
		-s USE_SDL=2 -s WASM=1 \
		-s INITIAL_MEMORY=33554432 \
		-s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]'

screenshot: $(BIN)
	@mkdir -p screenshots
	./$(BIN) $(ARGS)

generate-matrix: $(BIN)
	./generate_matrix.sh

clean:
	rm -rf $(BUILD) $(BIN) $(BIN_GUI)

# ---- Build rules ----

$(BUILD):
	@mkdir -p $(BUILD)

define compile_rule
$(call obj_name,$(1)): $(1) | $(BUILD)
	$(CC) $(CFLAGS) -c $$< -o $$@
endef

# GUI-only sources (sim_sdl2.c) need SDL2 include paths
define gui_compile_rule
$(call obj_name,$(1)): $(1) | $(BUILD)
	$(CC) $(GUI_CFLAGS) -c $$< -o $$@
endef

GUI_ONLY_SRCS := $(filter-out $(ALL_SRCS),$(ALL_SRCS_GUI))

$(foreach src,$(ALL_SRCS),$(eval $(call compile_rule,$(src))))
$(foreach src,$(GUI_ONLY_SRCS),$(eval $(call gui_compile_rule,$(src))))
