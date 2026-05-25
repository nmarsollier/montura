# Continue Development Rules

## Project: Montura - ESP32 Telescope Mount Controller

### Architecture (strict layers)
```
REST API → Mount → Motors → TMC2209 (Hardware)
```

### Key Rules
1. Microsteps come from TMC driver ONLY - call `tmc2209_get_active_microsteps()`
2. Never hardcode `DEFAULT_MICROSTEP` or `DEG_PER_MICROSTEP` - use inline helpers from `motors_internal.h`
3. One `.c` file per function/feature, named after the function
4. Prefix functions by module: `motors_`, `mount_`, `tmc_`, `motors_motion_`
5. Header guards: `#pragma once`
6. Naming: snake_case for functions/variables, UPPER_CASE for macros/defines
7. Logging: ESP_LOGx with static TAG

### Hardware Pins
- RA STEP: GPIO 26, RA DIR: GPIO 33
- DEC STEP: GPIO 25, DEC DIR: GPIO 32
- ENABLE: GPIO 27 (active low)
- TMC UART: GPIO 17 (TX), GPIO 16 (RX)

### Motion Timing
- STEP pulse: 2µs high + 2µs low
- Tracking (slow): vTaskDelay mode
- Slewing (fast): busy-wait with esp_timer
- Threshold: 20ms step period

### Common Mistakes to Avoid
- ✗ Including `motors_internal.h` from non-motors code
- ✗ Reading/writing TMC registers without CRC verification
- ✗ Blocking the motion task with long delays during slewing
- ✗ Modifying `motors_state` directly from mount/REST layers
