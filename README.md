# 🟢 Knobby Life Counter 

This is a non-commercial hobby project. Contributors are not associated with any company.

> [!Note]
> Full disclosure: this was programmed with the help of  generative AI

## ⚡ Features

Features/intended use:
- Life counter for Magic: the Gathering or other TCGs
- Life tracking from -999 to 999 with delta being shown as preview for 4 seconds
- Commander damage for up to 4 players, damage to all players
- Game timer (hours:minutes) and turn counter
- Brightness and battery guesstimate (WIP)
- D20 dice roll

## 🚀 Getting Started

TODO - cover menu interaction, etc, etc

## ⚙️ Hardware

This is the hardware I’ve used: JC3636K518 with battery, you can find it on AliExpress.

Specifications:
1.8 inch display, resolution 360*360
Display driver: ST77916
Touch: CST816
CPU: 240 MHz
Platform: ESP32

The one from waveshare might be identical: 
https://www.waveshare.com/wiki/ESP32-S3-Knob-Touch-LCD-1.8

## 🛠️ Building and Flashing

### Using arduino-cli

Install [arduino-cli](https://docs.arduino.cc/arduino-cli/installation/)

**Dependencies:**
```bash
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install lvgl@8.3.11
arduino-cli lib install ESP32_Display_Panel@1.0.0
arduino-cli lib install ESP32_IO_Expander@1.0.1
arduino-cli lib install esp-lib-utils@0.1.2
```

**Compile:**
```bash
arduino-cli compile \
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,FlashMode=qio" \
```

**Flash** (replace {port} with your device's port from `arduino-cli board list`):
```bash
arduino-cli upload \
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,FlashMode=qio" \
  -p {port} \
  knobby
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md)