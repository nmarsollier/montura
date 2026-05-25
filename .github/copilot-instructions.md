# Copilot Instructions — montura

Estas reglas son obligatorias para cualquier cambio en este repositorio.

## Definicion del negocio

- Ver archivo README.md
- Mantener definiciones de negocio en README.md y en docuementacion dentro del proyecto.
- Al ser un proyecto personal, se documenta en castellano, pero el codigo es en ingles, incluidos los textos impresos y
  logs.

## Contexto del proyecto

- Firmware ESP-IDF (C/FreeRTOS) para montura ecuatorial.
- Arquitectura modular en `main/`: modulos separados en subcarpetas por funcionalidad.
- Internamente cada modulo es un directorio con muchos archivos .c.
- Mantenemos una estructura minimalista de headers. Cada directorio contienen un solo header .h En casos donde queramos
  mantener cosas internas y reusarlas , usamos un _internal.h , pero nunca un .h por archivo .c
- Es programacion funcional, siempre intentamos que sea sigan las reglas de programacion funcional.
- El runtime orquesta, los módulos implementan lógica específica.
- Internamente a los modulos principales, se dividen siempre los .c en casos de uso.
- Es un proyecto chico, no existe el concepto de dominio, la app entera es un solo dominio.
- Se mantiene cada archivo .c enfocado en un solo caso de uso.

## Reglas de alcance

- Hacer cambios **enfocados** al requerimiento del usuario.
- No introducir features no solicitadas. Salvo que tengan sentido en el contexto del pedido del cambio y se solicita
  autorizacion.
- No refactorizar áreas no relacionadas. Salvo que tengan sentido en el contexto del pedido del cambio y se solicita
  autorizacion.
- No agregar commits/branches salvo pedido explícito.

## Reglas de carpetas y archivos

- Ignorar completamente `.history/` para búsquedas, edición y validación.
- No editar `build/` ni artefactos generados.
- No editar `sdkconfig`/`sdkconfig.old` salvo pedido explícito.

## Reglas funcionales vigentes (evitar regresiones)

- No reintroducir el módulo `sensors/` ni referencias a sensores HAL.
- No reintroducir el concepto de HOME por sensores/finales de carrera.
- El botón HOME sigue existiendo y su comportamiento vigente es: `mount_start_goto(0.0f, home_dec, 1)` donde `home_dec`
  es `+90` en hemisferio norte y `-90` en hemisferio sur (según `settings.lat`).
- No reintroducir guiado fino por guiders.
- No reintroducir `pulse-guide`:
    - No endpoint `POST /api/pulse-guide`.
    - No tipos/funciones de `GuideDirection`.
    - No handlers ni capacidades asociadas a `guide_direction`.
- Si una solicitud futura contradice estas reglas, solicitar confirmación explícita antes de aplicar el cambio.

## API y contrato

- Mantener contrato REST consistente:
    - `200` para soliciudes exitosas.
    - `400` para body inválido/faltante.
    - `409` para rechazo por reglas de estado.

## Convenciones de implementación

- Preservar estilo C existente (nombres, formato, comentarios en español).
- Mantener separación por responsabilidad en cada modulo. No mezclar responsabilidades en distintos modulos.

## Validación

- Tras cambios de código, ejecutar build ESP-IDF y verificar que compile limpio.
- Si hay errores no relacionados al cambio, reportarlos sin alterar alcance.

## Documentación

- Si se cambia API/flujo funcional, actualizar `README.md` y `api/mount.http` en el mismo cambio.

## Testing

- No se realizan archivos de test por el momento

## Reglas de IA

- Mantener este archivo actualizado cuando el usuario solicita que algo se debe tener en cuenta relacionado a la IA.
