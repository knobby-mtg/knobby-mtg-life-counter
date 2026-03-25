# Knobby Firmware System Specification

This document describes the current runtime architecture, hardware assumptions, and implementation-level behavior of the firmware.

## 1. Platform and Tooling

### 1.1 Target platform

- MCU family: ESP32-S3
- Framework: Arduino
- Display resolution: `360 x 360`
- Display controller: ST77916
- Touch controller: CST816S

### 1.2 Expected library set

The current PlatformIO configuration depends on:

- GFX Library for Arduino `^1.6.5`
- LVGL `^8.4.0`
- CST816S `^1.3.0`
- esp32_io_expander `^1.1.1`
- ESP32_Display_Panel `^1.0.4`
- SensorLib `^0.3.1`

The codebase and README both assume LVGL `8.4.x` compatibility.

## 2. Firmware Entry and Main Loop

### 2.1 Entry point

The Arduino sketch entry file is responsible for:

- CPU frequency selection
- Disabling wireless subsystems
- Calling board/UI initialization
- Entering the LVGL service loop

### 2.2 Setup sequence

The startup sequence is:

1. Set CPU frequency to `240 MHz`
2. Disable Wi-Fi
3. Stop Bluetooth
4. Delay `200 ms`
5. Start serial output at `115200`
6. Call `scr_lvgl_init()`
7. Call `knob_gui()`

### 2.3 Main loop behavior

The main loop repeatedly:

1. Processes queued knob events via `knob_process_pending()`
2. Executes `lv_timer_handler()`
3. Delays for `5` RTOS ticks via `vTaskDelay(5)`

This separates rotary interrupt/timer activity from UI mutation by queueing knob events and consuming them in the main UI loop.

## 3. Hardware Pin Assumptions

The current board configuration uses the following pin assignments:

- TFT backlight: `47`
- TFT reset: `21`
- TFT chip select: `14`
- TFT QSPI clock: `13`
- TFT data lines: `15`, `16`, `17`, `18`
- Touch I2C SCL: `12`
- Touch I2C SDA: `11`
- Touch interrupt: `9`
- Touch reset: `10`
- Rotary encoder A: `8`
- Rotary encoder B: `7`
- Battery ADC input: `1`

Additional SD and audio-related pin definitions exist in the board header but are not part of the current user-facing feature set.

## 4. Display Initialization

Display and touch initialization are implemented in `scr_st77916.h`.

### 4.1 Display stack

The firmware configures:

- An `ESP_PanelBusQSPI` bus for the ST77916 display
- An `ESP_PanelLcd_ST77916` panel instance
- A custom ST77916 vendor command table
- LVGL display driver callbacks for framebuffer flushing

### 4.2 Buffering model

- LVGL is initialized with a single draw buffer
- The buffer height is `72` rows
- Buffer allocation uses internal 8-bit capable memory via `heap_caps_malloc`
- Display flushing is delegated to `lcd->drawBitmap(...)`
- For non-RGB buses, draw completion triggers `lv_disp_flush_ready(...)`

### 4.3 Rotation support

The code includes a `setRotation()` helper that synchronizes display and touch orientation across four rotation states. The current startup path does not call it, so default orientation is used.

## 5. Touch Input Initialization

Touch input uses:

- `ESP_PanelBusI2C`
- `ESP_PanelTouch_CST816S`
- LVGL pointer input driver registration

Current behavior:

- Reads a single touch point per poll
- Reports pressed state when at least one point is read
- Reports released state otherwise
- Optional touch interrupt callback is installed but currently returns immediately without additional behavior

## 6. Rotary Encoder Integration

Rotary support is implemented in two layers:

- A low-level bidirectional rotary driver in `bidi_switch_knob.c`
- Application-level event queueing and screen-specific interpretation in `knob.c`

### 6.1 Low-level encoder behavior

The low-level driver shall:

- Configure encoder GPIOs as input with pull-up
- Poll both encoder phases using an ESP timer every `3 ms`
- Apply a debounce threshold of `2` ticks
- Increment or decrement an internal count value based on phase changes
- Emit `KNOB_LEFT` and `KNOB_RIGHT` callbacks

### 6.2 LVGL encoder bridge

The display init path also registers an LVGL encoder input device. Its current role is limited because the application does not use LVGL group-navigation for UX.

The actual user-visible behavior comes from direct calls to `knob_change(...)` from the low-level knob callbacks.

### 6.3 Application event queue

Application-level knob events are enqueued in a ring buffer with these properties:

- Queue size: `32` events
- Overflow strategy: drop the oldest event by advancing the tail
- Processing limit per loop: `8` events

This design prevents direct UI mutation from the encoder timer callback path.

## 7. UI Architecture

The entire current UI state machine is implemented in `knob.c`.

### 7.1 Screen set

The current firmware builds and retains all screens at startup:

- Intro screen
- Settings screen
- Multiplayer overview screen
- Multiplayer menu screen with player and global action variants
- Multiplayer rename screen
- Multiplayer commander-damage target selection screen
- Multiplayer commander-damage edit screen
- Multiplayer all-damage screen

### 7.2 Navigation model

- Screens are created once during startup
- The intro screen transitions directly to the multiplayer overview
- Navigation is implemented by loading prebuilt screens with `lv_scr_load(...)`
- UI refresh functions update labels, colors, and state before screen load where needed

### 7.3 State ownership

All major product state is held in file-scope static variables in `knob.c`, including:

- Brightness percentage
- Battery voltage and percent cache
- Multiplayer life totals, names, selection, and commander-damage matrix
- Multiplayer menu mode for player-specific vs. global actions
- Multiplayer pending preview state

There is no persistence layer in the current implementation. All state is reset on reboot.

## 8. Timing Model

The UI uses LVGL timers for several behaviors.

### 8.1 Timer inventory

- Intro animation: `500 ms`
- Multiplayer life preview commit: `4000 ms`

### 8.2 Commit semantics

Multiplayer life preview is implemented as a delayed commit rather than an immediate write. The user sees the pending delta first, and the committed total changes only when the timer expires.

### 8.3 Immediate-write exceptions

These flows update life totals immediately rather than via preview timer:

- Multiplayer commander damage edits
- Multiplayer all-damage apply
- Global reset

## 9. Battery Measurement Implementation

Battery sampling is implemented in the sketch file rather than `knob.c`.

### 9.1 Sampling details

- ADC attenuation: `ADC_11db`
- Sample count per read: `16`
- Inter-sample delay: `250 us`
- Outlier rejection: discard one minimum and one maximum sample
- Average based on remaining `14` samples

### 9.2 Scaling details

- Divider ratio constant: `2.0`
- Calibration scale: `1.0`
- Calibration offset: `0.0`

### 9.3 Filtering details

- First valid reading seeds the filtered value
- Subsequent readings use exponential smoothing with coefficients `0.7` old and `0.3` new

### 9.4 UI integration

- Battery percentage is derived from voltage using piecewise-linear interpolation
- Readings are cached for `5 seconds` in the settings flow

## 10. Brightness Control Implementation

Brightness is handled in two places:

- Display init configures a LEDC PWM channel for the panel backlight pin
- UI logic in `knob.c` also initializes and drives the same backlight pin through LEDC

Current application behavior uses the `knob.c` brightness path for user adjustments.

### 10.1 PWM configuration

- LEDC mode: low-speed
- Timer: `LEDC_TIMER_0`
- Channel: `LEDC_CHANNEL_0`
- PWM frequency: `5000 Hz`
- Resolution: `13-bit`
- Duty range: `0..8191`

## 11. Commander Damage Data Model

Commander damage is stored only in the multiplayer data model.

- Representation: a full `4 x 4` source-to-target matrix
- Meaning: `damage[source][target]`
- Initial value: all entries start at `0`

When the player menu for target `T` opens commander-damage selection:

- The UI lists all valid sources `S != T`
- Each row shows `damage[S][T]`
- The editor screen mutates `damage[S][T]`

## 12. Multiplayer Menu Modes

The multiplayer overview uses one menu screen implementation with two interaction modes.

- Long-press on quadrant `Q` opens the player menu for player `Q`
- The player menu exposes `Rename`, `Commander`, and `Back`
- Upward swipe on the multiplayer overview opens the global multiplayer menu
- The global menu exposes `Global`, `Settings`, `Reset`, and `Back`
- Selecting `Settings` opens the settings screen
- Selecting `Reset` restores in-memory defaults and returns to the multiplayer overview

## 13. Reset Model

The global reset path restores in-memory state only. It does not:

- Reinitialize hardware
- Recreate LVGL objects
- Reboot the MCU
- Persist or reload settings from storage

## 14. Known Behavioral Constraints of Current Implementation

These are not recommendations; they are current-system characteristics that any future change should evaluate explicitly.

- No persistent storage for brightness, names, or gameplay values
- No encoder push-button behavior in the UI flow
- No confirmation step before global reset
- Battery percentage is only an estimate based on a hard-coded voltage curve and uncalibrated constants

## 15. Source of Truth

The current implementation is primarily defined by:

- `knobby/knobby.ino` for startup loop and battery sampling
- `knobby/scr_st77916.h` for display, touch, and encoder initialization
- `knobby/knob.c` for application state, navigation, and feature logic
- `knobby/bidi_switch_knob.c` for low-level rotary polling and debounce