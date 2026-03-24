# Knobby Product Requirements

This document defines the intended product behavior of the current Knobby firmware at a higher level than the implementation-derived specifications. It is meant to act as a stable product reference for future refactoring and feature work.

## 1. Product Summary

Knobby is a handheld tabletop game utility built around a rotary knob and touch display. Its primary purpose is to provide fast, readable life tracking for Magic: the Gathering and similar games on a compact battery-powered device.

The product shall prioritize:

- Fast numeric adjustment with minimal taps
- Clear at-a-glance readability during play
- Support for both solo and multiplayer tracking scenarios
- Simple interaction flows suitable for tabletop use

## 2. Primary Use Cases

The product shall support these core use cases:

- Track a single player's life total during a game
- Track commander damage from multiple opponents
- Track life totals for up to four players on one device
- Track turn count and elapsed game time
- Roll a d20 when needed during play
- Adjust screen brightness on-device
- Show a rough battery estimate to the user

## 3. Core Product Requirements

### 3.1 Single-player life tracking

The product shall:

- Start a new session with a default life total of `40`
- Allow life totals to increase and decrease in single-step increments
- Support negative life totals
- Prevent values from exceeding the supported display range
- Present life values in a large, high-contrast format suitable for tabletop viewing

### 3.2 Fast adjustment model

The product shall:

- Use rotary input as the primary method for numeric changes
- Apply one unit of change per encoder step
- Provide immediate visual feedback while values are being adjusted

### 3.3 Commander damage tracking

The product shall:

- Allow the user to track commander damage from up to three opponents in single-player mode
- Show the accumulated damage per opponent
- Keep commander damage editing separate from the main life display flow
- Reflect commander damage changes in the relevant life total behavior

### 3.4 Multiplayer tracking

The product shall:

- Support four players simultaneously on one screen
- Show each player's name and life total
- Allow the user to select which player's life total is being adjusted
- Provide player-specific menu actions
- Allow player names to be edited on-device

### 3.5 Multiplayer commander damage

The product shall:

- Allow tracking commander damage relationships between players in multiplayer mode
- Allow the user to choose the relevant players involved in a commander-damage entry
- Show stored damage totals for those relationships

### 3.6 Global damage operation

The product shall:

- Support applying the same damage amount to all multiplayer players in one action

### 3.7 Turn tracking

The product shall:

- Support starting a turn counter and elapsed timer during a session
- Show the current turn number
- Show elapsed time in a readable format
- Allow advancing to the next turn from the main screen

### 3.8 Dice utility

The product shall:

- Provide a d20 roll utility
- Display the generated result clearly
- Allow returning to the main game flow quickly

### 3.9 Settings and device information

The product shall:

- Provide an on-device brightness control
- Show the current brightness value
- Show a battery estimate when available

### 3.10 Reset behavior

The product shall:

- Provide a way to reset gameplay-related state to default values
- Reset life totals, multiplayer state, commander damage state, timer state, and transient values

## 4. Interaction Requirements

### 4.1 Touch interaction

The product shall use touch input for:

- Screen navigation
- Menu selection
- Choosing players and actions
- Confirming or returning from secondary flows

### 4.2 Rotary interaction

The product shall use rotary input for:

- Life adjustment
- Damage adjustment
- Brightness adjustment
- Other numeric entry flows

### 4.3 Minimal-friction navigation

The interaction model should:

- Avoid deep navigation for common actions
- Make core gameplay actions reachable in one or two interactions from the main screen
- Keep return paths obvious from secondary screens

## 5. Display Requirements

The user interface shall:

- Be optimized for a `360 x 360` display
- Keep important values readable from tabletop viewing distance
- Use distinct visual states for different screens and players
- Preserve legibility across the supported life range

The interface should:

- Favor strong contrast over visual density
- Emphasize the currently active or selected target

## 6. Runtime and Device Constraints

The product shall:

- Run on the target ESP32-S3 display board used by this project
- Operate without requiring network access during normal gameplay
- Use LVGL `8.4.x` compatible behavior

The product should:

- Keep interaction latency low enough to feel immediate during play
- Remain usable on battery power

## 7. Data and Session Requirements

The current product requirement baseline is session-scoped behavior.

The product shall:

- Maintain state during runtime
- Reset to defaults on reboot unless persistence is explicitly added later

Persistence of settings or game state is currently out of scope for the baseline requirements.

## 8. Out of Scope

The following are not baseline requirements for the current product definition:

- Network features
- Cloud synchronization
- Multi-device communication
- Long-term save/load of sessions
- Audio features
- Rules enforcement beyond score and damage tracking

## 9. Relationship to Detailed Specs

This document defines the product-level intent.

The following documents remain the source for implementation-specific behavior and current-system detail:

- `functional-spec.md` for observable current behavior
- `system-spec.md` for runtime architecture, hardware assumptions, and implementation constraints