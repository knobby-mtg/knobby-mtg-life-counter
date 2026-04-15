#!/usr/bin/env sh
# config.sh - Knobby simulator configuration
# Auto-detects tools. Override any variable in config.local.sh (gitignored).

# ── LVGL ─────────────────────────────────────────────────────────────────────
# Try standard per-platform Arduino library paths
if [ -z "$LVGL_PATH" ]; then
    for candidate in \
        "$HOME/Documents/Arduino/libraries/lvgl" \
        "$HOME/Arduino/libraries/lvgl" \
        "$HOME/AppData/Local/Arduino15/libraries/lvgl" \
        "/usr/share/arduino/libraries/lvgl" \
        "/usr/local/share/arduino/libraries/lvgl"; do
        if [ -d "$candidate" ]; then
            LVGL_PATH="$candidate"
            break
        fi
    done
fi

if [ -z "$LVGL_PATH" ]; then
    echo "ERROR: LVGL not found. Set LVGL_PATH in config.local.sh" >&2
    exit 1
fi

# ── Python ────────────────────────────────────────────────────────────────────
if [ -z "$PYTHON" ]; then
    if command -v python3 >/dev/null 2>&1; then
        PYTHON=python3
    elif command -v python >/dev/null 2>&1; then
        PYTHON=python
    else
        echo "ERROR: Python not found in PATH." >&2
        exit 1
    fi
fi

# ── C Compiler ────────────────────────────────────────────────────────────────
if [ -z "$CC" ]; then
    if command -v clang >/dev/null 2>&1; then
        CC=clang
    elif command -v gcc >/dev/null 2>&1; then
        CC=gcc
    else
        echo "ERROR: No C compiler (clang/gcc) found in PATH." >&2
        exit 1
    fi
fi

# ── Make ──────────────────────────────────────────────────────────────────────
if [ -z "$MAKE" ]; then
    if command -v make >/dev/null 2>&1; then
        MAKE=make
    elif command -v mingw32-make >/dev/null 2>&1; then
        MAKE=mingw32-make
    else
        echo "ERROR: make not found in PATH." >&2
        exit 1
    fi
fi

# ── emcc (optional, only for WASM build) ─────────────────────────────────────────────
if [ -z "$EMCC" ]; then
    if command -v emcc >/dev/null 2>&1; then
        EMCC=emcc
    elif [ -x "$SCRIPT_DIR/emsdk/upstream/emscripten/emcc" ]; then
        EMCC="$SCRIPT_DIR/emsdk/upstream/emscripten/emcc"
    fi
fi

# If using the local emsdk, also set EMSDK_PYTHON to avoid macOS Xcode Python 3.9
if [ -z "$EMSDK_PYTHON" ] && [ -d "$SCRIPT_DIR/emsdk/python" ]; then
    _PYBIN=$(ls "$SCRIPT_DIR/emsdk/python/"|sort|tail -1)
    _PYCANDIDATE="$SCRIPT_DIR/emsdk/python/$_PYBIN/bin/python3"
    if [ -x "$_PYCANDIDATE" ]; then
        EMSDK_PYTHON="$_PYCANDIDATE"
    fi
    unset _PYBIN _PYCANDIDATE
fi

# ── User overrides ────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
if [ -f "$SCRIPT_DIR/config.local.sh" ]; then
    [ "$VERBOSE" = "1" ] && echo "[DEBUG] Loading config.local.sh"
    . "$SCRIPT_DIR/config.local.sh"
fi

if [ "$VERBOSE" = "1" ]; then
    echo "[DEBUG] Detected Configuration:"
    echo "  - LVGL_PATH:   $LVGL_PATH"
    echo "  - PYTHON:      $PYTHON"
    echo "  - CC:          $CC"
    echo "  - MAKE:        $MAKE"
    echo "  - EMCC:        ${EMCC:-Not Found}"
    echo "  - EMSDK_PYTHON:${EMSDK_PYTHON:-Not Found}"
fi

export LVGL_PATH PYTHON CC MAKE EMCC EMSDK_PYTHON
