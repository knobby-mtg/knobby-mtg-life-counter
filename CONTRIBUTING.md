# Contributing

Thanks for your interest in contributing to this project — contributions small and large are welcome.

## Code of conduct
Be respectful and collaborative. Treat others with courtesy; abusive or discriminatory behavior will not be tolerated.

## Report an issue
- Search existing issues first.
- Provide a clear title, steps to reproduce, expected vs actual behavior, and relevant hardware/OS/build details.

## Building and Flashing

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
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,FlashMode=qio,PartitionScheme=custom" \
```

The `PartitionScheme=custom` flag picks up `knobby/partitions.csv` (a copy of the Arduino core's `default_16MB` layout — 6.25MB APP / 3.43MB SPIFFS).

**Flash** (replace {port} with your device's port from `arduino-cli board list`):
```bash
arduino-cli upload \
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,FlashMode=qio,PartitionScheme=custom" \
  -p {port} \
  knobby
```

## Contribute code
1. Fork the repo and create a short-lived branch (feature/bugfix/your-name).
2. Keep changes focused and include tests or a short validation step when possible.
3. Follow the existing code style and naming patterns.
4. Commit messages: use the imperative present tense ("Add", "Fix", "Update").
5. Open a pull request against `main` referencing the issue (if any) and a short description of the change.

## Pull request checklist
- Runs and builds locally (see README for build instructions).
- Tested on hardware to run successfully
- Includes a short description and motivation.
- Adds or updates tests/examples where applicable.
- No unrelated changes or large formatting-only diffs.

## Testing & verification
Run your normal build/test flow before submitting. If you changed functionality, include steps to validate the change in the PR description.

## Questions
If you're unsure how to proceed, open an issue or contact a maintainer for guidance.

Thanks — we appreciate your help keeping this project healthy and useful.

