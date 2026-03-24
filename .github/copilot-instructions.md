Purpose
This repository contains firmware for an ESP32-based knob display (knobby). These workspace instructions give AI assistants quick context for contributing, building, and testing without duplicating README content.

How to use this assistant
- Ask a goal-oriented question (e.g., "Add a brightness toggle").
- Specify target tooling: Arduino CLI, PlatformIO, or manual Make/Makefile.

Quick Links
- Project README: [README.md](README.md)
- Firmware entry: [knobby/knobby.ino](knobby/knobby.ino)
- Core sources: [knobby/knob.c](knobby/knob.c), [knobby/knob.h](knobby/knob.h)
- HAL and helpers: [knobby/hal/](knobby/hal/)

Build & Flash (examples)
- Arduino CLI (ensure board packages installed):
  - `arduino-cli compile --fqbn esp32:esp32:esp32s3 ./knobby`
  - `arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32s3 ./knobby`
- PlatformIO:
  - `platformio run -e <env>`
  - `platformio run -e <env> -t upload`

Dependencies & environment notes
- Target: ESP32 (ESP32-S3 variants used in development).
- LVGL: v8.4 is required (see `knobby/lv_conf.h`).
- Check README for exact library versions and hardware notes.

Where to look when changing code
- `knobby/` — main firmware and config files (`knobby.ino`, `pincfg.h`, `lv_conf.h`).
- `knobby/hal/` — LVGL HAL implementations and `lv_hal.mk`.

Contributing (short)
- Keep LVGL API compatibility (v8.x) in mind.
- Build locally using Arduino CLI or PlatformIO and test on hardware.
- Create PRs with a short hardware verification note.

Example prompts
- "Search for all LVGL display init sequences and centralize them."
- "Add PlatformIO config for ESP32-S3 with LVGL 8.4 and document steps."
- "Make knob input debounce configurable via `pincfg.h`."
