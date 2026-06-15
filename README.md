# Monturita — No Precision Instruments Inc.

Fully functional mini equatorial mount toy powered by an ESP32, controllable via N.I.N.A. (Alpaca / ASCOM)
and its own REST API.

This firmware runs on an ESP32 NodeMCU board, driving two NEMA 17 stepper motors through
TMC2209 drivers. It exposes a full ASCOM Alpaca telescope interface on port 11111 so
N.I.N.A. and other clients can discover and control the mount directly.

## Architecture

```
N.I.N.A. / ASCOM client
Alpaca REST API  (port 11111)  ◄── also: UDP discovery on 32227
REST API  (port 80)  ── serves embedded SPA at /
  Mount  (service layer of orchestration, coordinates, settings)
  Motors  (move / track)
  TMC2209 UART driver  (hardware config & microstep control)
```

## Hardware

- **Board**: ESP32 NodeMcu (USB-C, CP2102, 38 pins)
- **Motor drivers**: 2× TMC2209 (UART, StealthChop, 128 µsteps)
- **Motors**: 2× NEMA 17 (1.8° step, 1.4 A rated)
- **Reduction**: 20-tooth motor pulley → 80-tooth axis pulley (4:1)
- **Buttons**: STOP (GPIO 18), HOME (GPIO 19) — active-low with internal pull-ups
- **LED**: Blue on-board LED (GPIO 2, active-high)

### Pin mapping

| GPIO | Function        |
|------|-----------------|
| 16   | TMC2209 UART RX |
| 17   | TMC2209 UART TX |
| 18   | STOP button     |
| 19   | HOME button     |
| 23   | Led Indicator   |
| 25   | DEC STEP        |
| 26   | RA STEP         |
| 27   | MOTORS ENABLE   |
| 32   | DEC DIR         |
| 33   | RA DIR          |

## Setup

### Requirements

| Tool    | Version      | Purpose                       |
|---------|--------------|-------------------------------|
| ESP-IDF | v6.0.1       | Firmware build system         |
| Python  | 3.10+ (venv) | Required by ESP-IDF tools     |
| CMake   | 4.x          | Build system                  |
| Ninja   | 1.x          | Build executor                |
| Node.js | 22+          | Web UI build (`www/build.js`) |
| npm     | 9+           | UI dependencies (Alpine.js)   |

### macOS install

```sh
# ESP-IDF v6.0.1
mkdir -p ~/.espressif
git clone --depth 1 --branch v6.0.1 https://github.com/espressif/esp-idf.git ~/.espressif/v6.0.1/esp-idf
export IDF_TOOLS_PATH="$HOME/.espressif/tools"
cd ~/.espressif/v6.0.1/esp-idf && bash install.sh esp32

# build tools + Node.js
brew install cmake ninja node
cd www && npm install
```

Add to `~/.zshrc` (adjust paths to match your system):

```sh
export IDF_PATH="$HOME/.espressif/v6.0.1/esp-idf"
export IDF_TOOLS_PATH="$HOME/.espressif/tools"
export IDF_PYTHON_ENV_PATH="$HOME/.espressif/tools/python/v6.0.1/venv"
export PYTHON="$IDF_PYTHON_ENV_PATH/bin/python"
alias idf.py="$PYTHON $IDF_PATH/tools/idf.py"
```

### Build

```sh
idf.py set-target esp32
idf.py build flash monitor
```

### Web UI

The SPA lives in `www/src/` (HTML, CSS, JS). Rebuild the embedded UI with:

```sh
node www/build.js
idf.py build
```

The resulting `www/dist/index.html` is embedded into the firmware via `EMBED_TXTFILES`.

## Project conventions

- Language: **C** (C23), snake_case
- One `.c` file per use case within each module
- Public API: `module.h` — Internal API: `module_internal.h`
- Function prefix matches module name (`motors_`, `mount_`, `alpaca_bridge_`, …)
- Dependencies: REST → Mount → Motors → Motors Motion → TMC (no reverse deps)
