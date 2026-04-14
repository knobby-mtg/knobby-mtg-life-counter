# 🟢 Knobby Life Counter 

This is a non-commercial hobby project. Contributors are not associated with any company.

> [!Note]
> Full disclosure: this was programmed with the help of generative AI

## ✨ Features

Features/intended use:
- Life counter for Magic: the Gathering or other TCGs
- Life tracking from -999 to 999 with delta being shown as preview for 4 seconds
- Support for 1 to 4 players
- Commander damage for up to 4 players, damage to all players
- Game timer (hours:minutes) and turn counter
- Brightness and battery guesstimate (WIP)
- D20 dice roll
- Event log

## 🛠️ Installation

Go to https://knobby-mtg.github.io/knobby-mtg-life-counter/ to install the latest release with a compatible browser.

> [!Warning]
> Installation is at your own risk, we take no responsiblity for failed installations or devices.

## 🚀 Getting Started

Swipe inward from any edge on a player screen to open the menu, and swipe down or in from the right edge to close a menu or go back.

## ⚙️ Hardware

This is the hardware used: JC3636K518 with battery, you can find it on AliExpress.

Specifications:
- 1.8 inch display, resolution 360*360
- Display driver: ST77916
- Touch: CST816
- CPU: 240 MHz
- Platform: ESP32

The one from waveshare might be identical: 
https://www.waveshare.com/wiki/ESP32-S3-Knob-Touch-LCD-1.8

## 🔧 Building

All build commands run from the repo root:

```bash
# First time: install Arduino cores and libraries
make firmware-deps

# Compile firmware for ESP32-S3
make firmware

# Flash to device (replace port with yours from `arduino-cli board list`)
make firmware-flash PORT=/dev/ttyACM0
```

## 📸 Screenshots

A headless simulator renders the UI without hardware, producing PNG screenshots with a circular display mask matching the physical device.

```bash
# Single screenshot
make screenshot                                    # default main screen
make screenshot ARGS="--screen 4p --life 200,30,40,15 --names Alice,Bob,Charlie,Dave"

# Full test matrix (308 screenshots + index.html gallery)
make generate-matrix
```

Run `sim/knobby_sim --help` for all CLI options. See [sim/screenshots/README.md](sim/screenshots/README.md) for details.

## 🧑‍🤝‍🧑 Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md)
