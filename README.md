# Monturita, no Precision Instruments Inc.

Mini montura ecuatorial DIY con ESP32, controlable vía NINA (Alpaca/ASCOM) y API REST propia.

Este software es un firmware ESP-IDF para una mini montura ecuatorial de juguete compatible con flujo NINA (vía bridge
Alpaca/ASCOM).

## Arquitectura

```text
NINA
↓
Bridge ASCOM
↓
REST API
↓
ESP32 (ESP-IDF)
↓
Core de estado + Runtime
↓
Drivers de I/O (motores/botones)
```

## Setup

### Requisitos recomendados (VS Code)

- ESP-IDF extension (Espressif)
- C/C++
- CMake Tools

### Inicialización

1. Abrir terminal ESP-IDF (`ESP-IDF: Open ESP-IDF Terminal`)
2. Seleccionar target:

```sh
idf.py set-target esp32
```

1. Build:

```sh
idf.py build
```

### Ejecución en QEMU (opcional)

```sh
idf.py qemu monitor
```

Salir de monitor: `Ctrl + ]`

## Coordenadas y rangos

- Convención de arranque:
    - El usuario posiciona la montura apuntando al polo celestial antes de encender

## Runtime (scheduler)

Task principal genérico con períodos configurables (`runtime/runtime_config.c`):

- `main_loop_period_ms`: `20`
- `screen_update_period_ms`: `100`
- `inputs_poll_period_ms`: `50`

Input polling (cada 50ms):

- `STOP` (rising edge) -> `mount_stop()`

## REST API

#### GET

- `/` (raíz — sirve la UI embebida)
- `/api/status`

#### POST

- `/api/tracking`
- `/api/goto`
- `/api/move-axis`
- `/api/stop`
- `/api/home`
- `/api/park`
- `/api/unpark`
- `/api/sync`
- `/api/settings`

### `GET /api/status`

Respuesta ejemplo:

```json
{
  "status": "ready",
  "tracking": "sidereal",
  "ra": "12:20:42.00",
  "dec": "-32:07:12.00",
  "settings": {
    "time": "2026-05-28T09:00:00-03:00",
    "lat": -32.8908,
    "lon": -68.8272,
    "elevation": 750
  }
}
```

### `POST /api/tracking`

```json
{
  "tracking": "sidereal"
}
```

### `POST /api/goto`

```json
{
  "ra": 10.234,
  "dec": -20.123,
  "speed": 2
}
```

`speed` es opcional; rango válido `1..4`.(0.5x a 16x)

### `POST /api/move-axis`

Mueve un eje de la montura una cantidad relativa de grados (positivos o negativos).

```json
{
  "axis": "dec",
  "degrees": -5.5,
  "speed": 2
}
```

- `axis`: `ra` o `dec` (requerido).
- `degrees`: grados a mover; positivo o negativo (requerido). Para RA: 1h = 15°.
- `speed`: opcional; rango válido `1..4`.

### `POST /api/stop`

Sin body. Detiene cualquier movimiento en curso (`slewing`).

### `POST /api/home`

Sin body. Invoca la acción de HOME (equivalente a pulsar el botón HOME físico). El handler ejecuta `motors_home()` —
detiene movimiento y ordena el slewing a los ángulos de home según la configuración.

Respuesta ejemplo (respuesta estándar de comandos):

```json
{
  "ok": true,
  "message": "home invoked"
}
```

### `POST /api/park`

Sin body. Fuerza estado `parked` + `tracking=none`.

### `POST /api/unpark`

Sin body. Pasa a `ready` + `tracking=none` si estaba parked.

### `POST /api/sync`

```json
{
  "ra": 5.572,
  "dec": -22.183
}
```

### `POST /api/settings`

```json
{
  "time": "2026-05-28T09:00:00-03:00",
  "lat": -32.8908,
  "lon": -68.8272,
  "elevation": 750
}
```

## Contrato de respuestas

### Respuesta estándar de comandos

```json
{
  "ok": true,
  "message": "..."
}
```

### Códigos HTTP

- `200 OK`: comando aceptado
- `400 Bad Request`: body faltante o inválido
- `409 Conflict`: comando válido pero rechazado por estado actual

## Reglas principales de estado (resumen)

- No se permite cambiar tracking durante movimiento (`slewing`)
- `goto` no se permite en `parked`
- `sync` no se permite durante movimiento

## Pantalla y UI embebida (SPA)

La interfaz web es una SPA (Alpine.js) compilada a un único `dist/index.html` y embebida dentro del binario firmware
usando el mecanismo nativo `EMBED_TXTFILES` de ESP-IDF. El linker expone el HTML como símbolos C
(`_binary_index_html_start` / `_binary_index_html_end`) sin necesidad de archivos `.h` generados ni scripts Python.
El HTML embebido se sirve en la raíz del servidor (`GET /`),
de forma que la montura puede servir la interfaz incluso sin acceso a la red.

- Mapeo de controles de la UI a la REST API:
    - Botón `STOP` → `POST /api/stop` (detiene movimiento).
    - Botón `HOME` → `POST /api/home` (handler centralizado; implementado en `motors_home()`; reemplaza la antigua
      indirección `buttons_home_pressed()`).
    - Flechas de slew manual → `POST /api/move-axis` (usa `motors_slew_axis` / `mount_move_axis`).
    - Selector de tracking + Set → `POST /api/tracking` (usa `motors_start_tracking` / `mount_set_tracking`).
    - La UI lee estado desde `GET /api/status`.

Workflow de desarrollo de la UI

- Estructura:
    - Fuentes de desarrollo: `www/src/` (HTML parcial, CSS, JS).
    - Script de build simple: `www/build.js` — genera `www/dist/index.html` embebiendo CSS/JS en un único fichero.

- Pasos para regenerar y probar la UI embebida:
    1. Editar fuentes en `www/src/`.
    2. Generar `dist/index.html`:

```sh
node www/build.js
```

    3. Recompilar el firmware (`idf.py build`). El archivo `www/dist/index.html` se embebe automáticamente en el
       binario gracias a `EMBED_TXTFILES` en `main/CMakeLists.txt`.

## Notas de hardware

Hoy el proyecto usa mocks (`motors_mock`, `buttons_mock`).
Cuando llegue la placa, la migración esperada es reemplazar mocks por drivers reales, manteniendo estable el core y la
API.
