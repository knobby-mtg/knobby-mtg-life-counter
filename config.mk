# Knobby Simulator Configuration
# Automatically included by Makefiles.
#
# To override paths for your machine, create a file called config.local.sh
# (gitignored) and set any of these variables there.
# The shell scripts (sim.sh, compile.sh, sim-web.sh) will pass them to make.
#
# Example config.local.sh:
#   LVGL_PATH=/opt/homebrew/share/arduino/libraries/lvgl
#   CC=clang-17
#   EMCC=/opt/emsdk/upstream/emscripten/emcc

# C compiler — auto-detected by config.sh, passed in from shell
CC        ?= clang

# OS-Specific Defaults
ifeq ($(OS),Windows_NT)
    # Make command — use mingw32-make if make is missing
    MAKE_CMD := $(shell where make 2>nul)
    ifndef MAKE_CMD
        MAKE := mingw32-make
    else
        MAKE := make
    endif
    
    # LVGL path — Windows default
    LVGL_PATH ?= $(USERPROFILE)/Documents/Arduino/libraries/lvgl
    
    # Python — Windows prefers 'python' over 'python3'
    PYTHON    ?= python
    
    # Emscripten compiler — Windows requires .bat extension and cmd.exe wrapper
    ifneq ($(wildcard emsdk/upstream/emscripten/emcc.bat),)
        EMCC ?= cmd.exe /c "emsdk\upstream\emscripten\emcc.bat"
    else
        EMCC ?= cmd.exe /c emcc.bat
    endif
else
    MAKE      := make
    
    # OS detection for macOS
    UNAME_S := $(shell uname -s)
    
    # LVGL path — standard location on macOS vs Linux
    ifeq ($(UNAME_S),Darwin)
        LVGL_PATH ?= $(HOME)/Documents/Arduino/libraries/lvgl
    else
        LVGL_PATH ?= $(HOME)/Arduino/libraries/lvgl
    endif
    
    # Python — auto-detected as python3/python
    PYTHON    ?= python3
    
    # Emscripten compiler — passed in from shell when available, or auto-detected in repo
    CONFIG_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
    ifneq ($(wildcard $(CONFIG_DIR)emsdk/upstream/emscripten/emcc),)
        EMCC ?= $(CONFIG_DIR)emsdk/upstream/emscripten/emcc
        # Use emsdk-bundled Python 3.10+ so macOS Xcode Python 3.9 is not picked up
        EMSDK_PYTHON_CANDIDATES := $(wildcard $(CONFIG_DIR)emsdk/python/*/bin/python3)
        ifneq ($(EMSDK_PYTHON_CANDIDATES),)
            export EMSDK_PYTHON := $(lastword $(sort $(EMSDK_PYTHON_CANDIDATES)))
        endif
    else
        EMCC ?= emcc
    endif
endif

# Arduino CLI — standard install path (rarely needs changing)
ARDUINO_CLI ?= arduino-cli
