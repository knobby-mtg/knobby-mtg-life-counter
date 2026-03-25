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

Knobby is a non-commercial hobby project for tabletop games such as Magic: The Gathering. This fork narrows the project to multiplayer commander play and treats the original broader utility features as legacy behavior.

Hardware assumptions, pin mappings, and platform details are documented in [docs/specifications/system-spec.md](./docs/specifications/system-spec.md).

## Fork Direction

- Focus on multiplayer commander support for `2` to `4` players
- Keep multiplayer life totals and commander-damage tracking as the core experience
- Add commander tax tracking as a first-class game action
- Simplify the UI around multiplayer-only flows
- Remove single-player functionality from the product direction
- Remove auxiliary features such as the timer and d20 roller

## Planned Scope

- Multiplayer-only game flow
- Commander damage between players
- Commander tax tracking
- Support for pods from `2` to `4` players
- Touch and rotary input tuned for multiplayer interactions

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

- The current codebase still contains legacy features from the upstream project
- This README reflects the intended direction of this fork, not a claim that every planned change is already implemented
- Current implementation details are documented in [docs/specifications/functional-spec.md](./docs/specifications/functional-spec.md)

## Current Limitations

- The multiplayer commander-focused fork scope is not fully implemented yet
- Battery percentage is an estimate based on a fixed voltage curve
- State is not persisted across reboots
- LVGL compatibility is intentionally conservative; upgrading beyond 8.4.x will require code changes
