# iLink Ceiling Fan RF Controller

This project integrates "P&B FAN" (or generic iLink) 433 MHz RF ceiling fans into Home Assistant using an ESP32 and a CC1101 transceiver, running ESPHome.

## Overview

Many generic BLDC ceiling fans (often unbranded or sold under various names, using a "P&B FAN" remote) use a 433 MHz RF protocol that is notoriously difficult to reproduce with cheap FS1000A/SYN115 transmitters due to strict timing requirements (260 µs base pulse). 

This project solves this by using a **CC1101** transceiver, which handles OOK (On-Off Keying) modulation in hardware, ensuring perfect timing and allowing reliable control from Home Assistant.

## Hardware Required

- 1× **ESP32** microcontroller (e.g., ESP32 DevKit V1)
- 1× **CC1101 433 MHz transceiver module** (ensure it's a 433 MHz version with an SMA antenna)
- Jumper wires

⚠️ **CRITICAL: Power the CC1101 from 3.3V ONLY.** Connecting it to 5V will permanently damage the chip. The ESP32's 3V3 pin is perfect for this.

## Wiring (ESP32 to CC1101)

| CC1101 Pin | ESP32 Pin | Notes |
|------------|-----------|-------|
| VCC        | 3V3       | **3.3V ONLY - DO NOT USE 5V** |
| GND        | GND       | |
| SCK        | GPIO18    | VSPI CLK |
| MISO       | GPIO19    | VSPI MISO |
| MOSI       | GPIO23    | VSPI MOSI |
| CSN        | GPIO5     | Chip Select |
| GDO0       | GPIO4     | TX data pin |
| GDO2       | GPIO2     | RX data pin (Optional, for capture) |

## Software & Configuration

This project is built on **ESPHome**. The core configuration is in `fan_controls/esphome_final.yaml`. 

1. Install ESPHome (e.g., via Home Assistant add-on or local Python environment).
2. Edit `fan_controls/esphome_final.yaml` to update your WiFi credentials and Home Assistant API settings if necessary.
3. Compile and flash the firmware to your ESP32:
   ```bash
   esphome run fan_controls/esphome_final.yaml
   ```

### Important ESPHome Configuration Note
Due to an open bug in ESPHome's CC1101 component regarding `gdo0_pin` routing, **do not** specify `gdo0_pin` in the `cc1101:` block. Let the `remote_transmitter` component own the TX pin (GPIO4) exclusively, and trigger the transmission using `cc1101.begin_tx` in `on_transmit`. This workaround is already implemented in the provided `.yaml` files.

## Home Assistant Integration

Once flashed, Home Assistant will automatically discover the ESPHome device. The `esphome_final.yaml` configuration exposes standard Fan entities (with speeds 1-6, forward/reverse, and breeze modes) that behave exactly like the physical remote.

## Adding New Fans (Capturing Codes)

If you have a different remote or need to capture new codes:
1. Flash `fan_controls/esphome_capture_cc1101.yaml` to your ESP32.
2. Monitor the ESPHome logs.
3. Press buttons on your physical remote. The logs will output the exact raw timings for each button.
4. Add these raw arrays to your `esphome_final.yaml` using the `remote_transmitter.transmit_raw` action.

*Historical research notes and failure analysis on cheap FS1000A transmitters can be found in `HYPOTHESES.md` and `CAPTURE_PLAN.md`.*
