# Knobby MTG Life Counter
# Top-level Makefile for firmware and simulator targets

-include config.mk

# Default paths if config.mk not present
MAKE ?= make

# ---- Arduino CLI ----
# ARDUINO_CLI is set by config.mk (defaults to 'arduino-cli' in PATH)
FQBN        := esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,FlashMode=qio,PartitionScheme=custom
EXTRA_URLS  := https://espressif.github.io/arduino-esp32/package_esp32_index.json

.PHONY: help firmware firmware-flash firmware-deps check-arduino \
        screenshot generate-matrix sim sim-gui sim-clean clean

help:
	@echo "Knobby MTG Life Counter"
	@echo ""
	@echo "Firmware (via arduino-cli):"
	@echo "  make firmware-deps                 - Install Arduino cores and libraries"
	@echo "  make firmware                      - Compile firmware for ESP32-S3"
	@echo "  make firmware-flash PORT=/dev/ttyACM0 - Flash firmware to device"
	@echo ""
	@echo "Screenshots (headless simulator):"
	@echo "  make screenshot                    - Screenshot main screen"
	@echo "  make screenshot ARGS=\"--screen 4p\" - Screenshot specific screen"
	@echo "  make generate-matrix               - Generate full screenshot matrix"
	@echo ""
	@echo "  make help                          - Show this message"
	@echo ""
	@echo "Interactive Simulator (PC):"
	@echo "  make sim                           - Build and run interactive SDL2 simulator"
	@echo "  make sim-gui                       - (alias for make sim)"
	@echo ""
	@echo "Interactive Simulator (Web):"
	@echo "  make sim-web-build                 - Build WASM simulator (requires Emscripten)"
	@echo "  make sim-web-run                   - Run Web simulator in browser (requires Python)"
	@echo ""
	@echo "Run sim/knobby_sim --help for all screenshot CLI options"
	@echo ""

# ---- Firmware targets ----

check-arduino:
	@if [ -z "$(ARDUINO_CLI)" ]; then \
		echo "Error: arduino-cli not found in PATH"; \
		echo "Install it from: https://docs.arduino.cc/arduino-cli/installation/"; \
		exit 1; \
	fi

firmware-deps: check-arduino
	$(ARDUINO_CLI) core update-index --additional-urls $(EXTRA_URLS)
	$(ARDUINO_CLI) core install esp32:esp32 --additional-urls $(EXTRA_URLS)
	$(ARDUINO_CLI) lib install lvgl@8.3.11
	$(ARDUINO_CLI) lib install ESP32_Display_Panel@1.0.0
	$(ARDUINO_CLI) lib install ESP32_IO_Expander@1.0.1
	$(ARDUINO_CLI) lib install esp-lib-utils@0.1.2

firmware: check-arduino
	$(ARDUINO_CLI) compile --fqbn "$(FQBN)" knobby

firmware-flash: check-arduino
	@if [ -z "$(PORT)" ]; then \
		echo "Error: specify PORT, e.g. make firmware-flash PORT=/dev/ttyACM0"; \
		echo "Available ports:"; \
		$(ARDUINO_CLI) board list; \
		exit 1; \
	fi
	$(ARDUINO_CLI) upload --fqbn "$(FQBN)" -p $(PORT) knobby

# ---- Simulator targets (delegate to sim/) ----

sim:
	$(MAKE) -C sim gui

sim-gui: sim

sim-web-build:
	$(MAKE) -C sim web

sim-web-run:
	@echo "[INFO] Avvio server web locale..."
	@echo "[INFO] Premi Ctrl+C per fermare il server."
	$(PYTHON) -m http.server 8000 --directory .

screenshot:
	$(MAKE) -C sim screenshot ARGS="$(ARGS)"

generate-matrix:
	$(MAKE) -C sim generate-matrix

sim-clean:
	$(MAKE) -C sim clean

clean: sim-clean
