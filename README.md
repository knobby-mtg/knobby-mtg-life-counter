Hi, this is a non-commercial hobby project. I'm not associated with any company.

This is the hardware I’ve used: JC3636K518 with battery, you can find it on AliExpress.
hardware:
1.8 inch display, resolution 360*360
display driver: ST77916
touch: CST816
cpu: 240 MHz
platform: ESP32

The one from waveshare might be identical: 
https://www.waveshare.com/wiki/ESP32-S3-Knob-Touch-LCD-1.8 I’ve flashed their demo firmware and it works.

features/intended use:
- life counter for Magic: the Gathering or other TCGs

- life tracking from 0 - 666

- commander damage for up to 4 players, damage to all players

- game timer (hours:minutes) and turn counter

- brightness and battery guesstimate (honestly, I could use some help with that)

- d20 dice

- some glitches because I reduced the Cpu from 240 to 160 MHz

Full disclosure: this was programmed with the help of a generative AI because I've only started to learn how to do this.
You're welcome to contribute, change stuff! I don't think I have the time or the skills to implement all the features that have been requested.
