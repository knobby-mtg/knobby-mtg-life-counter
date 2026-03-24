# Knobby Firmware Functional Specification

This document describes the behavior implemented in the current firmware. It is a descriptive specification of the existing codebase, not a proposal for future behavior.

## 1. Product Scope

The firmware implements a touch-and-rotary life counter for trading card games on an ESP32-S3 board with a 360x360 round display. The current product supports:

- A single-player life counter with deferred commit preview
- Commander damage tracking against up to three opponents
- A multiplayer mode for four players
- A turn timer and turn counter
- A d20 roller
- Display brightness control
- Battery voltage sampling and estimated percentage display

## 2. Startup Behavior

On boot, the firmware shall:

- Set the CPU frequency to 240 MHz
- Disable Wi-Fi and Bluetooth
- Initialize the display, touch controller, LVGL, and rotary encoder
- Initialize the UI and backlight control
- Show an intro screen that reveals the text `knobby.` one character every 500 ms
- Transition automatically to the main screen after the intro finishes

## 3. Main Screen

The main screen shall provide the default single-player life tracking interface.

### 3.1 Initial state

- Starting life total: `40`
- Displayed life range: `-999` to `999`
- Default brightness: `30%`
- Turn timer hidden and disabled
- Commander damage values reset to `0`

### 3.2 Life display

- The life total shall be rendered as a custom segmented numeric display.
- Positive preview values shall show a `+` sign.
- Negative totals shall show a `-` sign.
- The circular life arc shall clamp display output to the range `0..40`, even if actual life is outside that range.

### 3.3 Life color thresholds

The main life display color shall be selected from the committed life total as follows:

- `> 40`: purple
- `30..40`: green
- `11..29`: yellow
- `<= 10`: red

During pending preview, the display shall use:

- Green for positive pending deltas
- Red for negative pending deltas

### 3.4 Rotary input on main screen

- Rotate left: decrease pending life delta by `1`
- Rotate right: increase pending life delta by `1`
- Each encoder step represents exactly one life

### 3.5 Deferred commit behavior

Main-screen life changes shall not commit immediately.

- Rotary changes update a pending delta preview
- The preview timer shall reset on every encoder movement
- If no further life change occurs for `4 seconds`, the pending delta shall be committed to the life total
- The committed life total shall remain clamped to `-999..999`

## 4. Main Screen Touch Interactions

The main screen shall expose the following touch targets:

- Tap life display area: open commander damage player selection
- Tap top-center icon: open settings
- Tap bottom-center circular button: open the main radial-style menu
- Upward swipe with vertical movement greater than `80` pixels and horizontal drift less than `90` pixels: open the main radial-style menu
- Tap turn widget, when visible: advance the turn counter and restart turn timing from the current moment

## 5. Main Menu

The main menu shall appear as an overlay over the main screen.

### 5.1 Menu actions

- `reset`: restore all gameplay and UI state to defaults
- `back`: close the menu and return to the main screen
- `Commander`: open commander damage target selection
- `d20`: roll a 20-sided die and open the dice screen
- `multiplayer`: open four-player mode
- `timer`: start a fresh turn timer session

### 5.2 Overlay behavior

- The overlay shall be dismissed by tapping outside the central menu panel
- Rotary input on the main screen shall be ignored while the overlay is visible

## 6. Commander Damage Tracking

Commander damage is implemented as a three-target tracking flow tied to the single-player life counter.

### 6.1 Selection screen

The commander damage selection screen shall:

- Show three selectable player rows
- Display a name and accumulated damage value for each row
- Allow returning to the main screen via a `Back` button

### 6.2 Target naming behavior

By default, the target rows are labeled `P1`, `P2`, and `P3`.

If any multiplayer player name is set to exactly `m`, the commander damage target list shall exclude that player and reuse the remaining three multiplayer names as the commander damage labels.

This is current implementation behavior and acts as an implicit "main player" marker for commander damage display mapping.

### 6.3 Damage screen

Selecting a commander damage row shall open a damage screen for that target.

The damage screen shall:

- Show the selected target name
- Show the accumulated damage value
- Use the selected target's color theme as the background
- Allow returning to the main screen via a `Back` button

### 6.4 Rotary input on damage screen

- Rotate right: increase selected commander damage by `1`
- Rotate left: decrease selected commander damage by `1`, with a floor at `0`

### 6.5 Life coupling

Commander damage edits shall directly affect the single-player life preview:

- Increasing commander damage decreases the pending single-player life total by the same amount
- Decreasing commander damage increases the pending single-player life total by the same amount
- The life change still uses the same `4 second` deferred commit mechanism as normal life edits

## 7. Turn Timer

The firmware shall support a simple turn counter with elapsed time display.

### 7.1 Starting the timer

Selecting `timer` from the main menu shall:

- Set the turn number to `1`
- Reset elapsed time to `0`
- Show the turn widget on the main screen
- Enable live timing
- Start a temporary blink effect on the turn indicator area

### 7.2 Display format

- The turn label shall show `turn` when the counter is `0`
- Otherwise it shall show `turn N`
- Elapsed time shall be displayed as `hours:minutes`
- Seconds are not displayed

### 7.3 Turn advancement

Tapping the turn widget shall:

- Increment the turn number if already active
- Preserve elapsed time accumulated so far
- Restart live timing from the moment of the tap

### 7.4 Reset behavior

Global reset shall:

- Disable the timer
- Hide the turn widget
- Clear the turn number and elapsed time

## 8. Dice Screen

The dice feature shall behave as follows:

- Selecting `d20` from the main menu generates a random value from `1` to `20`
- The dice screen displays the rolled value in large text
- The screen also displays the hint `tap to return`
- Tapping the dice screen returns to the main screen and reopens the main menu overlay

## 9. Settings Screen

The settings screen shall provide brightness and battery information.

### 9.1 Entry

- The settings screen is opened from the top-center icon on the main screen

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

## 11. Multiplayer Mode

The firmware shall implement a four-player mode on a single 360x360 screen.

### 11.1 Layout

- The multiplayer screen shall contain four 180x180 quadrants
- Each quadrant represents one player
- Each quadrant shows a player name and life total
- Default names: `P1`, `P2`, `P3`, `P4`
- Default life totals: `40` for all players

### 11.2 Selection behavior

- Tapping a quadrant selects that player
- The selected player shall use a brighter version of that quadrant's base color
- Rotary input changes the selected player's life total only

### 11.3 Multiplayer life editing

Multiplayer life changes shall use a deferred commit preview similar to the main screen.

- Rotate left: decrease selected player's pending delta by `1`
- Rotate right: increase selected player's pending delta by `1`
- Preview changes commit after `4 seconds` of inactivity
- Multiplayer life totals are clamped to `-999..999`
- If the selected player changes while another player's preview is pending, the previous preview is committed immediately

### 11.4 Menu gesture

- An upward swipe with vertical movement greater than `80` pixels and horizontal drift less than `90` pixels shall open the multiplayer global menu for the currently selected player context

## 12. Multiplayer Player Menu

Long-pressing a multiplayer quadrant shall open that player's menu.

The player menu shall provide:

- `Rename`
- `Commander`
- `Back`

## 13. Multiplayer Global Menu

Swiping upward on the multiplayer overview shall open the multiplayer global menu.

The global menu shall provide:

- `Global`
- `Back`
- `Main`

Selecting `Main` shall return to the main screen and reopen the main overlay menu.

## 14. Multiplayer Rename Flow

The rename screen shall:

- Show `Rename <player name>` as the title
- Provide a one-line text area
- Limit names to `15` characters
- Provide an on-screen keyboard
- Provide `save` and `back` buttons

Saving shall behave as follows:

- Empty text resets the name to the default `Pn`
- Non-empty text replaces the player name exactly as entered
- All dependent screens shall refresh to reflect the new name

## 15. Multiplayer Commander Damage Flow

The multiplayer commander damage flow is organized around the currently opened player menu.

### 14.1 Victim-first selection model

- Opening `Commander` from a player's menu treats that menu player as the damage recipient
- The next screen lists all other players as possible sources of commander damage
- Each row displays the other player's name and the current stored damage total from that source to the selected recipient

### 14.2 Damage editing

Selecting a source player opens a damage editor screen with the title `<target> -> <source>`.

Rotary behavior:

- Rotate right: increase commander damage total by `1`
- Rotate left: decrease commander damage total by `1`, with a floor at `0`

Applying a change shall:

- Update the stored per-source/per-target commander damage matrix
- Immediately subtract the delta from the target player's committed life total
- Refresh multiplayer overview values and commander damage screens immediately

## 16. Multiplayer All-Damage Flow

The all-damage screen shall:

- Start with pending damage value `0`
- Allow knob-based adjustment with a floor at `0`
- Provide `apply` and `back` buttons

The all-damage flow shall be entered from the multiplayer global menu.

Selecting `back` on the all-damage screen shall return to the multiplayer global menu.

Selecting `apply` shall subtract the pending damage value from all four players immediately.

## 17. Reset Semantics

Global reset shall restore the following defaults:

- Single-player life total to `40`
- Pending previews cleared
- Brightness to `30%`
- Dice result to unset
- Commander damage totals cleared
- Multiplayer life totals to `40`
- Multiplayer names to `P1` through `P4`
- Multiplayer selection state reset to player `0`
- Turn timer disabled and hidden

## 17. Input Model Summary

The current product uses:

- Capacitive touch for navigation and screen interaction
- Rotary encoder rotation for all numeric adjustments

The current firmware does not implement a rotary encoder push action as part of the user-facing feature set.