# Reglas de Desarrollo del Proyecto Montura

Mantener este archivo en formato simple, para que pueda leerse y editarse rapidamente

## Definicion del proyecto

- Logica para manejar montura ecuatorial DIY simple
- Utiliza un modulo ESP32 NodeMcu ESP32 WiFi Bluetooth 4.2 con USB-C y 38 Pines (ver seccion "Placa ESP32" abajo para
  especificaciones completas y pinout)
- Utiliza 2 TMC2209 de BT https://global.bttwiki.com/TMC2209.html
- Utiliza 2 motores paso a paso Nema17 con las siguientes caracteristicas: Low Noise:15N.cm(21oz.in) holding torque,
  drive voltage 12V/ 24V, rated current 1.4A, resistance 3.5ohms
- Utiliza 2 poleas 20 dientes en el motor, 80 dientes en el eje de rotacion
- La montura posee 2 botones fisicos, Stop y Home
- Posee una pantalla minimalista OLED de 0.98 inch, se muestra verticalmente

## Placa ESP32

### Identificacion

- **Modelo**: NodeMcu ESP32 WiFi Bluetooth 4.2 con USB-C y 38 Pines
- **SoC**: ESP32 (Tensilica Xtensa 32-bit LX6, doble nucleo)
- **Chip USB-Serial**: CP2102
- **Factor de forma**: NodeMcu estandar
- **Dimensiones**: 51 x 25 mm
- **Antena**: Integrada en PCB

### Especificaciones tecnicas

- **CPU**: Tensilica Xtensa 32-bit LX6, doble nucleo, hasta 240 MHz
- **Desempeno**: Hasta 600 DMIPS
- **WiFi**: 802.11 b/g/n/e/i (WFA, WPA/WPA2, WAPI)
- **Bluetooth**: v4.2 BR/EDR + BLE (Bluetooth Low Energy)
- **Alimentacion**: 5V via USB-C
- **Logica I/O**: 3.3V (tolerancia de pines: solo 3.3V, no tolera 5V)
- **GPIO digitales**: 24 (algunos pines solo como entrada)
- **ADC**: Conversor analogico-digital integrado (12-bit SAR ADC, 18 canales)
- **DAC**: 2 canales DAC de 8 bits
- **UART**: 2 controladores UART
- **I2C**: 2 controladores I2C
- **SPI**: 2 controladores SPI (alta velocidad)
- **I2S**: 2 controladores I2S
- **PWM LED**: 16 canales PWM independientes
- **Sensores capacitivos touch**: 10 pines
- **Sensor de efecto Hall**: Integrado

### Memorias

- **ROM**: 448 KB
- **SRAM**: 520 KB
- **SRAM en RTC**: 16 KB
- **Flash SPI externa**: 4 MB (QSPI)

### Seguridad hardware

- Estandares IEEE 802.11: WFA, WPA/WPA2, WAPI
- OTP de 1024 bits
- Aceleracion criptografica por hardware: AES, HASH (SHA-2), RSA, ECC, RNG

### Layout completo de la placa con conexiones

```
LADO IZQUIERDO               LADO DERECHO
(USB-C abajo)                (USB-C abajo)

3V3  ← TMC VIO (3.3V)        GND  ← TMC GND
EN   (reset, no conectar)    G23  (libre)
SP   (GPIO36, solo in)       G22  (libre, I2C SCL)
SN   (GPIO39, solo in)       TXD  (GPIO1, UART0 TX, no tocar)
G34  ← Hall sensor (futuro)  RXD  (GPIO3, UART0 RX, no tocar)
G35  ← Hall sensor (futuro)  G21  (libre, I2C SDA)
G32  ← DEC DIR               GND  ← GND botones
G33  ← RA DIR                G19  ← HOME button
G25  ← DEC STEP              G18  ← STOP button
G26  ← RA STEP               G5   (strapping, no tocar)
G27  ← ENABLE                G17  ← TMC TX (UART2)
G14  (libre)                 G16  ← TMC RX (UART2)
G12  (strapping, no tocar)   G4   (strapping, no tocar)
GND  ← 12V GND (fuente ext)  G0   (BOOT, no tocar)
G13  (libre)                 G2   (LED onboard)
SD2  (flash, no tocar)       G15  (strapping, no tocar)
SD3  (flash, no tocar)       SD1  (flash, no tocar)
CMD  (flash, no tocar)       SD0  (flash, no tocar)
V5   (5V USB, no usar)       CLK  (flash, no tocar)
```

### Pinout definitivo de Montura

| Label placa | GPIO | Funcion           | Modulo        | Notas                          |
|-------------|------|-------------------|---------------|--------------------------------|
| 3V3         | —    | TMC VIO / MS1/MS2 | tmc           | Alimentacion logica 3.3V       |
| GND (der)   | —    | TMC GND / Botones | tmc / buttons | Tierra comun                   |
| GND (izq)   | —    | 12V GND externo   | motors        | Tierra de fuente de motores    |
| G16         | 16   | TMC UART RX       | tmc           | UART2, single-wire bus         |
| G17         | 17   | TMC UART TX       | tmc           | UART2, single-wire bus         |
| G18         | 18   | STOP button       | buttons       | Input con pull-up interno      |
| G19         | 19   | HOME button       | buttons       | Input con pull-up interno      |
| G25         | 25   | DEC STEP          | motors_motion | Pulso STEP eje declinacion     |
| G26         | 26   | RA STEP           | motors_motion | Pulso STEP eje ascension recta |
| G27         | 27   | MOTORS ENABLE     | motors_motion | Enable global de ambos motores |
| G32         | 32   | DEC DIR           | motors_motion | Direccion eje declinacion      |
| G33         | 33   | RA DIR            | motors_motion | Direccion eje ascension recta  |

**Uso futuro:**
| G21 | 21 | I2C SDA | — | Sensor / display |
| G22 | 22 | I2C SCL | — | Sensor / display |
| G34 | 34 | Hall limit | — | Solo input, requiere pull-up ext |
| G35 | 35 | Hall limit | — | Solo input, requiere pull-up ext |

### Pines con restricciones (NO USAR)

| Label | GPIO | Restriccion                     |
|-------|------|---------------------------------|
| G0    | 0    | Boot: LOW en reset = modo flash |
| TXD   | 1    | UART0 TX, consola debug         |
| G2    | 2    | Strapping + LED onboard         |
| RXD   | 3    | UART0 RX, consola debug         |
| G4    | 4    | Strapping                       |
| G5    | 5    | Strapping (HIGH al boot)        |
| CLK   | 6    | Flash SPI interno               |
| SD0   | 7    | Flash SPI interno               |
| SD1   | 8    | Flash SPI interno               |
| SD2   | 9    | Flash SPI interno               |
| SD3   | 10   | Flash SPI interno               |
| CMD   | 11   | Flash SPI interno               |
| G12   | 12   | Strapping MTDI (voltaje flash)  |
| G15   | 15   | Strapping (LOW al boot)         |

### Notas de desarrollo para esta placa

- El chip CP2102 requiere drivers en macOS/Windows. En macOS los drivers son nativos desde Sierra.
- La placa usa USB-C para alimentacion y programacion. No requiere cable USB-A a micro-USB.
- Para flashear: mantener G0 a GND durante reset, o usar `idf.py flash` que lo hace automaticamente via DTR/RTS.
- El LED onboard esta en G2, activo HIGH. Puede usarse como indicador de estado.
- La antena en PCB ofrece buen rendimiento, pero si la montura tiene partes metalicas cercanas, considerar orientacion.
- El regulador de voltaje onboard convierte 5V de USB a 3.3V para el ESP32 y perifericos. La corriente maxima disponible
  en el pin 3V3 depende del regulador (tipicamente ~500-800 mA).

## Arquitectura

Capas del sistema, de afuera hacia adentro:

```
Cliente Web (Alpine.js) → REST API → Mount (orquestacion) → Motors → TMC2209 (hardware)
```

- **www/** — UI Web embebida programada con Alpine.js. Compila con `node www/build.js`, genera `www/dist/`.
- **REST API** (`main/rest/`) — Expone endpoints HTTP para control de la montura.
- **Mount** (`main/mount/`) — Orquestacion logica del montaje: estado, coordenadas, sincronizacion.
- **Runtime** (`main/runtime/`) — Inicializacion y ciclo de vida del sistema.
- **Motors** (`main/motors/`) — Control de motores de alto nivel.
- **Motors Motion** (`main/motors_motion/`) — Ejecucion hardware: generacion de pulsos STEP/DIR.
- **TMC** (`main/tmc/`) — Driver TMC2209 via UART. Unica fuente de verdad para configuracion de microsteps.
- **Buttons** (`main/buttons/`) — Manejo de botones fisicos.
- **Network** (`main/network/`) — Conectividad WiFi.
- **Tools** (`main/tools/`) — Utilidades transversales (parser, validacion).

## Reglas generales

- Nunca hacer commit o push a github sin autorizacion explicita
- Commits solo cuando la feature completa este verificada y compilando
- Siempre basarse en codigo desde el disco como unica fuente de verdad. El codigo fuente es la unica referencia valida
  para entender detalles de implementacion
- Antes de cualquier cambio leer el estado actual del archivo a modificar
- Proyecto programado en C, siguiendo estilo funcional cuando sea posible
- Modulos separados por dominio, cada dominio en una carpeta dentro de `main/`
- Cada dominio expone:
    - `dominio.h` — API publica (funciones que otros modulos pueden llamar)
    - `dominio_internal.h` — API interna (funciones compartidas solo entre archivos del mismo modulo)
- Un archivo `.c` por caso de uso dentro de cada modulo
- Las funciones publicas comienzan con el nombre del modulo, seguido de `_`
- Las variables de estado de un modulo nunca se acceden directamente desde afuera. Solo a traves de funciones publicas
  del modulo
- Respetar principios: DRY - YAGNI - KISS
- La documentacion en headers describe el problema de negocio o caso de uso que resuelve la funcion, no los detalles de
  implementacion
- Solo comentar codigo si es complejo de comprender para un humano
- El codigo y sus comentarios dentro de archivos con extension .c y .h se escriben en ingles
- Archivos markdown .md se escriben en idioma español
- Los tags de comentarios deben definirse como constante y su valor es el mismo nombre del archivo en mayuscular y sin
  la extension .c

## Convenciones de Codigo

- **Lenguaje**: C (no C++)
- **snake_case** para funciones y variables
- **UPPER_CASE** para macros y defines
- **Headers**: `#pragma once`, includes organizados: primeros los propios del modulo, luego librerias del framework
- **Logging**: usar `ESP_LOGI()`, `ESP_LOGW()`, `ESP_LOGE()` con tag estatico por archivo
- **Resultados entre capas**: usar los tipos `MotorResultCode` y `MountResult` definidos en el proyecto

## Relaciones entre modulos (dependencias)

- REST API puede llamar a Mount y Network
- Mount puede llamar a Motors
- Motors puede llamar a Motors Motion
- Motors Motion puede llamar a TMC
- TMC no depende de ningun otro modulo del proyecto (solo del framework ESP-IDF)
- Buttons y Screen son autocontenidos y pueden llamar a Mount
- Tools es transversal: no depende de modulos de dominio ni es dependido por ellos
