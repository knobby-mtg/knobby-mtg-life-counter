# Knobby Screenshot Simulator
# Requires: gcc, LVGL installed via arduino-cli (see CONTRIBUTING.md)

LVGL_PATH := $(HOME)/Arduino/libraries/lvgl
KNOBBY    := ../knobby
BUILD     := build
BIN       := knobby_sim

CC        := gcc
CFLAGS    := -Wall -Wno-unused-function \
             -DLV_CONF_INCLUDE_SIMPLE -DSIMULATOR=1 \
             -I. -Iesp_stubs -I$(LVGL_PATH) -I$(KNOBBY) \
             -O2 -g

LDFLAGS   := -lm

# ---- Source files ----

# Knobby UI/logic files (NOT: knobby.ino, board_detect.c, bidi_switch_knob.c)
KNOBBY_SRCS := $(addprefix $(KNOBBY)/, \
	knob.c knob_scr_main.c knob_scr_multiplayer.c knob_scr_menus.c \
	knob_intro.c knob_timer.c knob_life.c knob_damage_log.c \
	knob_game_mode.c knob_rename.c knob_dice.c knob_hw.c knob_nvs.c)

# Sim files
SIM_SRCS := sim_main.c sim_stubs.c

# LVGL sources (recursive wildcard)
rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
LVGL_SRCS := $(call rwildcard,$(LVGL_PATH)/src/,*.c)

ALL_SRCS := $(SIM_SRCS) $(KNOBBY_SRCS) $(LVGL_SRCS)

# Object files — flatten into build/ using path-mangled names
obj_name = $(BUILD)/$(subst /,__,$(subst ..,UP,$(1:.c=.o)))
ALL_OBJS := $(foreach src,$(ALL_SRCS),$(call obj_name,$(src)))

# ---- Targets ----

.PHONY: screenshot generate-matrix clean check-lvgl

$(BIN): check-lvgl $(ALL_OBJS)
	$(CC) $(ALL_OBJS) -o $@ $(LDFLAGS)

screenshot: $(BIN)
	@mkdir -p screenshots
	./$(BIN) $(ARGS)

generate-matrix: $(BIN)
	./generate_matrix.sh

clean:
	rm -rf $(BUILD) $(BIN)

check-lvgl:
	@if [ ! -d "$(LVGL_PATH)/src" ]; then \
		echo "Error: LVGL not found at $(LVGL_PATH)"; \
		echo "Install it with: make firmware-deps (from repo root)"; \
		echo "(See CONTRIBUTING.md for full setup)"; \
		exit 1; \
	fi

# ---- Build rules ----

$(BUILD):
	@mkdir -p $(BUILD)

define compile_rule
$(call obj_name,$(1)): $(1) | $(BUILD)
	$(CC) $(CFLAGS) -c $$< -o $$@
endef

$(foreach src,$(ALL_SRCS),$(eval $(call compile_rule,$(src))))
