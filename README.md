Hi, this is a non-commercial hobby project. I'm not associated with any company.

This is the hardware I’ve used: JC3636K518 with battery, you can find it on AliExpress.
hardware:
1.8 inch display, resolution 360*360
display driver: ST77916
touch: CST816
cpu: 240 MHz
platform: ESP32

IMPORTANT: this uses LVGL library version 8.4 the newest version breaks stuff.

The one from waveshare might be identical: 
https://www.waveshare.com/wiki/ESP32-S3-Knob-Touch-LCD-1.8 I’ve flashed their demo firmware and it works.

features/intended use:
- life counter for Magic: the Gathering or other TCGs

- life tracking from -999 to 999 with delta being shown as preview for 4 seconds

- commander damage for up to 4 players, damage to all players

- game timer (hours:minutes) and turn counter

- brightness and battery guesstimate (honestly, I could use some help with that)

- d20 dice

- cpu is back at 240 MHz to reduce glitches, testing real life battery performance right now.

Full disclosure: this was programmed with the help of a generative AI because I've only started to learn how to do this.
You're welcome to contribute, change stuff! I don't think I have the time or the skills to implement all the features that have been requested.

## Building and Flashing (Work in Progress)

### Using arduino-cli

Install [arduino-cli](https://docs.arduino.cc/arduino-cli/installation/), then install the ESP32 core and required libraries:
```
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install lvgl@8.3.11
arduino-cli lib install ESP32_Display_Panel@1.0.0
arduino-cli lib install ESP32_IO_Expander@1.0.1
arduino-cli lib install esp-lib-utils@0.1.2
```

**Compile:**
```
arduino-cli compile \
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,FlashMode=qio" \
  knobby
```

**Flash** (replace the port with your device's port from `arduino-cli board list`):
```
arduino-cli upload \
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,FlashMode=qio" \
  -p /dev/cu.usbmodem1 \
  knobby
```
