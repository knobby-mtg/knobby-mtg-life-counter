# Knobby Firmware Hardening Specification

This document defines a concrete reliability hardening plan for the current firmware. It focuses on reducing crash risk, startup fragility, input inconsistency, and timing-related edge cases without changing the product feature set.

## 1. Purpose

The hardening effort shall target the highest-risk reliability issues identified in the current codebase:

- Unsynchronized encoder driver state shared with an ESP timer callback
- Partial hardware initialization paths that continue into UI startup
- Dual ownership of rotary input through both application and LVGL paths
- Silent knob event loss under load
- Rotary decoding that is more noise-sensitive than necessary

The goal is to make the firmware more predictable on real hardware, easier to debug, and safer to extend.

## 2. Non-Goals

This hardening plan does not require:

- New user-facing gameplay features
- Persistent storage
- Network connectivity
- A redesign of the visual interface
- Full RTOS task refactoring across the entire application

Where possible, changes should preserve existing UX behavior.

## 3. Hardening Objectives

The firmware shall meet the following reliability objectives after this work:

- Startup either completes successfully or fails in a controlled and diagnosable manner
- Rotary input has a single authoritative event path for product behavior
- Hardware callback code shall not mutate shared runtime structures without synchronization
- Input overload shall be observable and controlled rather than silently corrupting behavior
- Encoder behavior shall remain stable under fast turns and typical switch bounce

## 4. Workstreams

### 4.1 Startup and Initialization Safety

The startup path shall be refactored so that initialization results are explicit and checked.

#### Requirements

- `scr_lvgl_init()` shall return a success/failure result instead of failing silently
- `setup()` shall stop normal UI startup if display, touch, LVGL, or knob initialization fails
- Hardware initialization steps shall validate critical return values where the current code does not
- Allocation failure paths shall clean up any partially initialized resources that were created earlier in the same function
- The system shall emit actionable serial log messages for each initialization failure class

#### Acceptance criteria

- UI construction does not proceed after LVGL display registration failure
- UI construction does not proceed after knob creation failure
- A failed touch or display bring-up does not leave the firmware in a partially initialized main loop
- Failure mode is deterministic across repeated boots with the same fault present

### 4.2 Encoder Driver Synchronization

The low-level encoder driver shall protect shared structures accessed by both timer and non-timer code.

#### Requirements

- Access to the global knob handle list shall be synchronized
- Knob creation and deletion shall not race with the periodic encoder timer callback
- Timer lifecycle transitions shall be safe when the first knob is created or the last knob is deleted
- The design shall prevent traversal of freed knob nodes

#### Preferred implementation direction

- Use a critical section, spinlock, or equivalent ESP-IDF-safe synchronization primitive around handle-list mutation and iteration
- Alternatively, stop the encoder timer before list mutation and restart it only after the list is stable again

#### Acceptance criteria

- No code path can free a knob node while the timer callback may still traverse it
- No code path can insert a partially initialized knob node into the shared list
- Repeated create/delete cycles do not crash or corrupt the list

### 4.3 Single Rotary Input Ownership Model

Rotary behavior shall be routed through one authoritative application input path.

#### Requirements

- The firmware shall choose exactly one of these models:
  - Direct application queue handling from knob callbacks
  - LVGL encoder input handling with application behavior derived from LVGL state
- The non-authoritative path shall be removed or reduced to a passive adapter that cannot also drive behavior
- Shared state such as `ctx_diff` shall not be used as an unsynchronized side channel between callback code and LVGL polling code

#### Preferred implementation direction

- Keep the existing application queue as the authoritative path because current product behavior is already screen-specific in `knob.c`
- Remove or disable the extra LVGL encoder registration unless it is required for a future group-navigation feature

#### Acceptance criteria

- A single encoder step results in one product-level action
- Fast direction changes do not produce duplicated or missing actions from competing input paths
- Rotary behavior remains consistent regardless of LVGL input polling cadence

### 4.4 Knob Event Queue Robustness

The application knob queue shall degrade predictably under burst input.

#### Requirements

- Queue overflow shall be observable through logging, counters, or diagnostics
- Overflow behavior shall be explicitly documented in code and spec comments
- Queue handling shall be reviewed against the current `8` events per loop processing cap and the `5` tick loop delay
- The design shall prefer coalescing or bounded backpressure over silent loss when practical

#### Preferred implementation direction

- Add an overflow counter and surface it through serial logging in debug builds
- Evaluate either increasing drain capacity, reducing loop delay, or coalescing repeated left/right steps before discard

#### Acceptance criteria

- Input loss, if it still occurs, is measurable rather than silent
- Rapid knob rotation no longer appears randomly unresponsive without diagnostics
- Queue behavior is documented and testable

### 4.5 Rotary Decoding Reliability

The low-level rotary algorithm shall be hardened against bounce and ambiguous phase transitions.

#### Requirements

- Encoder direction detection shall be based on valid quadrature state transitions rather than loosely independent channel events where possible
- Debounce behavior shall be documented and configurable if needed
- Invalid or contradictory transitions shall be ignored instead of generating false left/right events

#### Preferred implementation direction

- Replace the current per-channel event logic with a compact quadrature state machine
- Preserve the existing product contract of one logical step per user detent as closely as hardware allows

#### Acceptance criteria

- Quick alternating turns do not generate obvious wrong-direction steps
- Normal switch bounce does not produce repeated phantom events
- Encoder behavior is stable enough to use on all current value-adjustment screens

## 5. Execution Order

The work should be executed in the following order:

1. Startup and initialization safety
2. Encoder driver synchronization
3. Single rotary input ownership model
4. Knob event queue robustness
5. Rotary decoding reliability

This sequence reduces the risk of debugging higher-level input behavior on top of an unsafe startup or driver foundation.

## 6. Verification Plan

Each workstream shall include both code-level verification and hardware behavior checks.

### 6.1 Code verification

- Build the firmware successfully with the project's primary toolchain
- Confirm no new compiler warnings are introduced in touched areas
- Review all new failure paths for resource cleanup and logging coverage

### 6.2 Hardware verification

- Verify successful boot to the intro screen followed by the multiplayer overview on target hardware
- Verify controlled behavior when the encoder is turned rapidly in both directions
- Verify brightness adjustment remains responsive on the settings screen
- Verify commander damage and multiplayer life adjustments still map one encoder step to one logical value change
- Verify no crashes occur during repeated screen transitions and prolonged idle runtime

### 6.3 Fault injection checks

Where practical, simulate or force these conditions:

- LVGL draw buffer allocation failure
- Display registration failure
- Knob creation failure
- Queue overload from rapid encoder activity

The firmware shall fail safely or report useful diagnostics for each case.

## 7. Deliverables

The hardening effort shall produce:

- Updated firmware code implementing the reliability changes
- Updated system specification describing the new startup and input architecture
- Inline code comments only where necessary to explain non-obvious synchronization or queue behavior
- A short hardware verification note summarizing what was tested on-device

## 8. Definition of Done

This hardening effort is complete when:

- The highest-risk concurrency issue in the encoder driver is removed
- Startup no longer continues after critical initialization failure
- Rotary input has one authoritative path for application behavior
- Queue overload behavior is visible and intentional
- Encoder handling is materially more robust on hardware without regressing current UX
