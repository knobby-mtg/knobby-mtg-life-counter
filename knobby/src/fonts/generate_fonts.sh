#!/bin/bash
# Font generation script for Knobby MTG Life Counter
# Requires lv_font_conv on PATH (install via: npm install -g lv_font_conv)
#
# Usage:  ./generate_fonts.sh
# Re-run after changing sizes, weights, or character ranges below.

set -euo pipefail
cd "$(dirname "$0")"

CONV=lv_font_conv
BPP=4
FORMAT=lvgl
LV_INCLUDE="lvgl.h"

# Toggle compression: "true" = smaller flash, more CPU per glyph render
# "false" = larger flash, zero decompression overhead
COMPRESS=false

# Font source files
BOLD=Montserrat-Bold.ttf
REGULAR=Montserrat-Regular.ttf

# ---------- Character ranges ----------
# Digits + signs for life total / dice (space, +, -, 0-9, =)
RANGE_DIGITS="0x20,0x2B,0x2D,0x30-0x39,0x3D"

# Full printable ASCII (for general UI text)
RANGE_ASCII="0x20-0x7F"

# ---------- Compression flag ----------
COMPRESS_FLAG=""
if [ "$COMPRESS" = "true" ]; then
    COMPRESS_FLAG="--no-compress false"
else
    COMPRESS_FLAG="--no-compress"
fi

# ---------- Font definitions ----------
# Each entry: output_name font_file weight size range
generate_font() {
    local name="$1"
    local font="$2"
    local size="$3"
    local range="$4"
    local outfile="${name}.c"

    echo "Generating ${outfile} (size=${size}, bpp=${BPP})..."
    $CONV \
        --font "$font" \
        --bpp "$BPP" \
        --size "$size" \
        --range "$range" \
        --format "$FORMAT" \
        $COMPRESS_FLAG \
        --lv-include "$LV_INCLUDE" \
        -o "$outfile"
}

# ---------- Generate fonts ----------

# Large bold font for life total and dice result
generate_font "lv_font_montserrat_bold_116" "$BOLD" 116 "$RANGE_DIGITS"

# Regular font for life preview total ("= xxx")
generate_font "lv_font_montserrat_regular_48" "$REGULAR" 48 "$RANGE_DIGITS"

# Bold fonts for multiplayer life totals (56 for absolute/tabletop, 44 for centric)
generate_font "lv_font_montserrat_bold_56" "$BOLD" 56 "$RANGE_DIGITS"
generate_font "lv_font_montserrat_bold_44" "$BOLD" 44 "$RANGE_DIGITS"

echo ""
echo "Done! Generated fonts:"
ls -lh *.c 2>/dev/null || echo "(no .c files found)"
