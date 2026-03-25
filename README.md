<div align="center">

# Knobby MTG Life Counter

Firmware for a compact life counter built for round ESP32-S3 knob display boards.

[Overview](#overview) • [Fork Direction](#fork-direction) • [Build and Flash](#build-and-flash)

</div>

> [!IMPORTANT]
> This firmware is currently tied to LVGL 8.4.x. Newer LVGL releases are not drop-in compatible with this codebase.

> [!NOTE]
> This repository is a fork of [knobby-mtg/knobby-mtg-life-counter](https://github.com/knobby-mtg/knobby-mtg-life-counter). This fork keeps the original firmware as its base.

## Overview

Knobby is a non-commercial hobby project for tabletop games such as Magic: The Gathering. This fork is now a multiplayer-only commander life counter and no longer includes the upstream single-player, timer, or d20 utility flows.

Hardware assumptions, pin mappings, and platform details are documented in [docs/specifications/system-spec.md](./docs/specifications/system-spec.md).

## Fork Direction

- Focus on multiplayer commander support on a shared pod-tracking screen
- Keep multiplayer life totals and commander-damage tracking as the core experience
- Add commander tax tracking as a first-class game action
- Simplify the UI around multiplayer-only flows

## Current Behavior

- Boot into the intro screen, then transition directly to the multiplayer overview
- Track life totals for four players on one screen
- Support commander damage tracking between players
- Support global damage application, player renaming, brightness control, and battery estimate display
- Exclude single-player, timer, and d20 flows from the shipped UI

## Out of Scope

- Single-player life counter flow
- Timer / turn tracking
- d20 roller
- General-purpose utility behavior outside commander play

## Build and Flash

PlatformIO is the simplest way to build this project because the dependencies are already declared in [platformio.ini](./platformio.ini).

### PlatformIO

```bash
platformio run -e esp32s3
platformio run -e esp32s3 -t upload
```

### Arduino CLI

If you prefer Arduino tooling, install the required ESP32 board support and libraries first, then run:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3 ./knobby
arduino-cli upload -p <PORT> --fqbn esp32:esp32:esp32s3 ./knobby
```

## Notes

- The current codebase is intentionally scoped to the multiplayer-only fork direction
- Upstream history may still reference removed features, but they are no longer part of the current firmware behavior
- Current implementation details are documented in [docs/specifications/functional-spec.md](./docs/specifications/functional-spec.md)

## Current Limitations

- The current firmware is built around a fixed four-player layout
- Battery percentage is an estimate based on a fixed voltage curve
- State is not persisted across reboots
- LVGL compatibility is intentionally conservative; upgrading beyond 8.4.x will require code changes
