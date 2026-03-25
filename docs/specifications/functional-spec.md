# Knobby Firmware Functional Specification

This document describes the behavior implemented in the current firmware. It is a descriptive specification of the existing codebase, not a proposal for future behavior.

## 1. Product Scope

The firmware implements a touch-and-rotary life counter for trading card games on an ESP32-S3 board with a 360x360 round display. The current product supports:

- A multiplayer mode for four players
- Multiplayer commander damage tracking between players
- Multiplayer all-player damage application
- On-device multiplayer renaming
- Display brightness control
- Battery voltage sampling and estimated percentage display

## 2. Startup Behavior

On boot, the firmware shall:

- Set the CPU frequency to 240 MHz
- Disable Wi-Fi and Bluetooth
- Initialize the display, touch controller, LVGL, and rotary encoder
- Initialize the UI and backlight control
- Show an intro screen that reveals the text `knobby.` one character every 500 ms

- Transition automatically to the multiplayer overview after the intro finishes

## 3. Multiplayer Overview

The multiplayer overview is the default gameplay screen.

### 3.1 Initial state

- Default life totals: `40` for all players
- Default names: `P1`, `P2`, `P3`, `P4`
- Selected player: `P1`
- Default brightness: `25%`
- Commander damage totals reset to `0`

### 3.2 Layout

- The overview contains four `180 x 180` quadrants
- Each quadrant represents one player
- Each quadrant shows a player name and life total

### 3.3 Selection behavior

- Tapping a quadrant selects that player
- The selected player shall use a brighter version of that quadrant's base color
- Rotary input changes the selected player's life total only

### 3.4 Multiplayer life editing

- Rotate left: decrease the selected player's pending life delta by `1`
- Rotate right: increase the selected player's pending life delta by `1`
- Preview changes commit after `4 seconds` of inactivity
- Multiplayer life totals are clamped to `-999..999`
- If the selected player changes while another player's preview is pending, the previous preview is committed immediately

### 3.5 Menu gestures

- Long-pressing a quadrant opens that player's player menu
- Swiping upward with vertical movement greater than `80` pixels and horizontal drift less than `90` pixels opens the multiplayer global menu

## 4. Multiplayer Player Menu

Long-pressing a multiplayer quadrant shall open that player's menu.

The player menu shall provide:

- `Rename`
- `Commander`
- `Back`

## 5. Multiplayer Global Menu

Swiping upward on the multiplayer overview shall open the multiplayer global menu.

The global menu shall provide:

- `Global`
- `Settings`
- `Reset`
- `Back`

Selecting `Back` shall return to the multiplayer overview.

## 6. Multiplayer Rename Flow

The rename screen shall:

- Show `Rename <player name>` as the title
- Provide a one-line text area
- Limit names to `15` characters
- Provide an on-screen keyboard
- Provide `Save` and `Back` buttons

Saving shall behave as follows:

- Empty text resets the name to the default `Pn`
- Non-empty text replaces the player name exactly as entered
- All dependent screens shall refresh to reflect the new name

## 7. Multiplayer Commander Damage Flow

The multiplayer commander damage flow is organized around the currently opened player menu.

### 7.1 Victim-first selection model

- Opening `Commander` from a player's menu treats that menu player as the damage recipient
- The next screen lists all other players as possible sources of commander damage
- Each row displays the other player's name and the current stored damage total from that source to the selected recipient

### 7.2 Damage editing

Selecting a source player opens a damage editor screen with the title `<source> -> <target>`.

Rotary behavior:

- Rotate right: increase commander damage total by `1`
- Rotate left: decrease commander damage total by `1`, with a floor at `0`

Applying a change shall:

- Update the stored per-source/per-target commander damage matrix
- Immediately subtract the delta from the target player's committed life total
- Refresh multiplayer overview values and commander damage screens immediately

## 8. Multiplayer All-Damage Flow

The all-damage screen shall:

- Start with pending damage value `0`
- Allow knob-based adjustment with a floor at `0`
- Provide `Apply` and `Back` buttons

The all-damage flow shall be entered from the multiplayer global menu.

Selecting `Back` on the all-damage screen shall return to the multiplayer global menu.

Selecting `Apply` shall subtract the pending damage value from all four players immediately.

## 9. Settings Screen

The settings screen shall provide brightness and battery information.

### 9.1 Entry

- The settings screen is opened from the multiplayer global menu

### 9.2 Brightness control

- The current brightness shall be displayed as a percentage
- A circular arc shall visualize the brightness value
- Rotary input changes brightness in steps of `1%`
- Brightness shall be clamped to `5..100%`
- Backlight PWM output shall be updated immediately when brightness changes

### 9.3 Battery display

- The screen shall display battery percentage and calibrated voltage detail when a valid reading exists
- If no valid reading exists, it shall show `Battery: --%` and `No calibrated reading`
- Battery sampling shall be refreshed when entering the settings screen
- Subsequent battery reads shall be cached for up to `5 seconds`

## 10. Battery Estimation Behavior

Battery estimation shall behave as follows:

- Sample ADC pin `1`
- Take `16` ADC millivolt samples per measurement
- Discard the minimum and maximum sample
- Average the remaining `14` samples
- Apply a divider ratio of `2.0`
- Apply calibration scale `1.0` and offset `0.0`
- Smooth measurements with a low-pass filter: `70%` previous value, `30%` new value
- Convert voltage to percentage using a piecewise-linear curve from `3.35V` to `4.18V`

## 11. Reset Semantics

Global reset shall restore the following defaults:

- Pending multiplayer previews cleared
- Brightness to `25%`
- Commander damage totals cleared
- Multiplayer life totals to `40`
- Multiplayer names to `P1` through `P4`
- Multiplayer selection state reset to player `0`

After reset, the firmware shall return to the multiplayer overview.

## 12. Input Model Summary

The current product uses:

- Capacitive touch for navigation and screen interaction
- Rotary encoder rotation for all numeric adjustments

The current firmware does not implement a rotary encoder push action as part of the user-facing feature set.