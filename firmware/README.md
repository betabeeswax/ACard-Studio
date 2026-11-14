# ACard OS — native firmware

This is the first native firmware foundation for the **M5Stack Cardputer ADV (K132-Adv)**. It is a separate ESP-IDF application; it does not replace the existing browser-based ACard Studio editor.

## Build

Use ESP-IDF 5.3 or newer with the ESP32-S3 target:

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py -p YOUR_SERIAL_PORT flash monitor
```

The GitHub Actions workflow builds this project for ESP32-S3 on every push and pull request.

## Current bring-up

- Native C++ application using ESP-IDF (not Arduino and not MicroPython).
- ST7789V2 display initialization over SPI and a minimal built-in text renderer.
- I2C bus probing for the ADV keyboard controller (TCA8418), BMI270 IMU and ES8311 audio codec.
- Optional microSD mounting using the ADV SPI pin map.
- On-device performance suite: CPU integer throughput, display fill throughput, free/largest heap block and microSD write throughput.
- Serial benchmark output uses a `BENCH,` CSV-style record.

## Benchmark meanings

- **CPU Mops/s**: 1.2 million deterministic integer-mixing iterations divided by elapsed microseconds. Use the same firmware build and power conditions for comparisons.
- **Display Mpixel/s**: pixels pushed during eight full-screen fills divided by elapsed time. This measures the SPI display path, not just CPU drawing speed.
- **Heap**: free 8-bit-capable heap and largest contiguous free block at the time of the run.
- **microSD kB/s**: sequential 256 KiB write test. It is skipped if no card is mounted.

Results are real only after flashing and running on the physical Cardputer ADV. The CI build cannot measure device performance.

## Important implementation status

This is an initial native OS bring-up, not a finished operating system. The keyboard controller and peripheral presence checks are in place, but full key mapping/input, BMI270 sensor initialization and reads, ES8311 audio playback/recording, Wi-Fi/BLE services, IR, power management, a window/app manager, and the ACard Studio block-to-native runtime still need implementation. The original Cardputer and Cardputer ADV have different hardware; this firmware uses the ADV pin map and devices.
