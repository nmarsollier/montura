# Monturita — No Precision Instruments Inc.

DIY mini equatorial mount powered by an ESP32, controllable via N.I.N.A. (Alpaca / ASCOM)
and its own REST API.

This firmware runs on an ESP32 NodeMCU board, driving two NEMA 17 stepper motors through
TMC2209 drivers.  It exposes a full ASCOM Alpaca telescope interface on port 11111 so
N.I.N.A. and other clients can discover and control the mount directly.

---

## Architecture

```
N.I.N.A. / ASCOM client
        │
        ▼
  Alpaca REST API  (port 11111)  ◄── also: UDP discovery on 32227
        │
        ▼
  Alpaca Bridge  (protocol ↔ mount translation)
        │
        ▼
  REST API  (port 80)  ── serves embedded SPA at /
        │
        ▼
  Mount  (orchestration, coordinates, settings)
        │
        ▼
  Motors  (high-level move / track / park)
        │
        ▼
  Motors Motion  (FreeRTOS task — step generation)
        │
        ▼
  TMC2209 UART driver  (hardware config & microstep control)
        │
        ▼
  GPIO STEP / DIR / EN  →  NEMA 17 stepper motors
```

Supporting modules:

| Module           | Purpose |
|------------------|---------|
| `buttons`        | Physical STOP and HOME button debounce + long-press detection |
| `led`            | On-board blue LED (GPIO 2) with timer-based auto-off |
| `network`        | Wi-Fi station / AP management, SNTP time sync |
| `runtime`        | App initialisation and main loop scheduler |
| `udp_alpaca`     | ASCOM Alpaca discovery responder (UDP port 32227) |
| `utils`          | Shared helpers (JSON, HTTP, string, number utilities) |

---

## Hardware

- **Board**: ESP32 NodeMcu (USB-C, CP2102, 38 pins)
- **Motor drivers**: 2× TMC2209 (UART, StealthChop, 128 µsteps)
- **Motors**: 2× NEMA 17 (1.8° step, 1.4 A rated)
- **Reduction**: 20-tooth motor pulley → 80-tooth axis pulley (4:1)
- **Buttons**: STOP (GPIO 18), HOME (GPIO 19) — active-low with internal pull-ups
- **LED**: Blue on-board LED (GPIO 2, active-high)

### Pin mapping

| GPIO | Function         |
|------|------------------|
| 16   | TMC2209 UART RX  |
| 17   | TMC2209 UART TX  |
| 18   | STOP button      |
| 19   | HOME button      |
| 25   | DEC STEP         |
| 26   | RA STEP          |
| 27   | MOTORS ENABLE    |
| 32   | DEC DIR          |
| 33   | RA DIR           |

---

## Setup

### Requirements (VS Code / CLion)

- ESP-IDF extension (Espressif) or ESP-IDF plugin
- C/C++ tooling
- CMake

### Build

```sh
idf.py set-target esp32
idf.py build
idf.py flash
idf.py monitor       # baud 115200
```

### Web UI

The SPA lives in `www/src/` (HTML, CSS, JS).  Rebuild the embedded UI with:

```sh
node www/build.js
idf.py build
```

The resulting `www/dist/index.html` is embedded into the firmware via `EMBED_TXTFILES`.

---

## REST API (port 80)

### GET

| Endpoint      | Description                    |
|---------------|--------------------------------|
| `/`           | Embedded SPA (index.html)      |
| `/api/status` | Full mount status & telemetry  |

### POST

| Endpoint          | Body                                                 |
|-------------------|------------------------------------------------------|
| `/api/tracking`   | `{"tracking":"sidereal"|"lunar"|"solar"|"none"}`   |
| `/api/goto`       | `{"ra":12.5,"dec":-32.1,"speed":2}`                  |
| `/api/move-axis`  | `{"axis":"ra"|"dec","degrees":5.0,"speed":2}`        |
| `/api/stop`       | _(empty)_                                            |
| `/api/home`       | _(empty)_                                            |
| `/api/park`       | _(empty)_                                            |
| `/api/unpark`     | _(empty)_                                            |
| `/api/sync`       | `{"ra":5.572,"dec":-22.183}`                         |
| `/api/settings`   | `{"lat":-32.89,"lon":-68.83,"elevation":750,"time":"2026-..."}` |
| `/api/wifi-config`| `{"ssid":"MyAP","password":"..."}`                   |

### Response contract

```json
{ "ok": true, "message": "..." }
```

HTTP status codes: `200` accepted, `400` bad request, `409` conflict.

### `GET /api/status`

```json
{
  "status": "ready",
  "tracking": "sidereal",
  "ra": "12:20:42.00",
  "dec": "-32:07:12.00",
  "time": "2026-06-09T22:00:00Z",
  "settings": {
    "lat": -32.8908,
    "lon": -68.8272,
    "elevation": 750
  },
  "wifi_ap": false,
  "is_home": true
}
```

- `status`: `ready`, `slewing`, `tracking`, `parked`, `disabled`
- `is_home`: `true` when RA = 0h 0m and DEC = 0° 0' and mount is ready

---

## Alpaca / ASCOM (port 11111)

Full telescope device at `/api/v1/telescope/0`.

Supported methods:

- `GET` properties: altitude, azimuth, side of pier, sidereal time, UTC date,
  slewing, tracking, at-home, at-park, site lat/lon/elevation, tracking rate, etc.
- `PUT` methods: slew to coordinates, sync to coordinates, slew to target,
  move axis, park, unpark, find home, set tracking, set UTC date, set site, etc.
- Management: `/management/apiversions`, `/management/description`,
  `/management/configureddevices`

UDP discovery on port 32227.

---

## N.I.N.A. notes

- **MoveAxis**: works with joystick-style directional controls.  Parameters
  (`Axis`, `Rate`) are read from both query string and form body.
- **Tracking**: set rate first (`TrackingRate`), then enable (`Tracking=true`).
  The rate is preserved across enable/disable cycles.
- **SlewToCoordinatesAsync**: maps to mount `goto` at 16 °/s with full
  acceleration / cruise / deceleration ramp profile.

---

## State rules

- Cannot change tracking during slewing
- `goto` / `home` / `park` rejected while status is not `ready`
- `sync` accepted in any state (aligns internal position model)
- STOP drains the motion queue so queued commands are discarded

---

## Runtime scheduler

Configurable periods in `runtime/runtime_config.c`:

| Period | Task                  |
|--------|-----------------------|
| 20 ms  | Main loop             |
| 50 ms  | Button polling        |
| 100 ms | Screen update         |

---

## Project conventions

- Language: **C** (C23), snake_case
- One `.c` file per use case within each module
- Public API: `module.h` — Internal API: `module_internal.h`
- Function prefix matches module name (`motors_`, `mount_`, `alpaca_bridge_`, …)
- Dependencies: REST → Mount → Motors → Motors Motion → TMC (no reverse deps)
