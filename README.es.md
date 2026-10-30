<p align="center">
  <img src="assets/banner.jpg" alt="Project RE: Visited - Resident Evil 3" width="100%">
</p>

# Project RE: Visited - Resident Evil 3

**Languages / Idiomas**: [English](README.md) | [Português (Brasil)](README.pt-br.md) | [**Español**](README.es.md)

---

**Port experimental para Windows de Resident Evil 3: Nemesis (PS1, SLUS-00923), construido sobre el framework de recompilación estática [PSXRecomp](https://github.com/RetroPortingToolKit/psxrecomp).**

> **Estado: en desarrollo / experimental.** Espera errores. Las notas de versión anteriores y este README exageraban lo que funciona; esta versión describe el proyecto tal como es realmente.

---

## Cómo funciona realmente

| Parte | Cómo se ejecuta |
|---|---|
| Ejecutable principal (`SLUS_009.23`, 3.328 funciones) | **Recompilado estáticamente** MIPS → C → x86-64. Se ejecuta como código nativo. |
| Overlays (código que el juego carga desde el CD para menús y enemigos) | **Recompilados con el mismo recompilador** en shards nativos (`cache/`). Desde la v1.7.0 la caché se genera **a partir del disco original**, no de sesiones de juego: los 17 overlays de menú y los 21 módulos de código de enemigos, 59 imágenes en total ([aot/README.md](aot/README.md)). Lo que aún falte se ejecuta en un intérprete pequeño la primera vez y se compila en segundo plano con el kit incluido (`overlay_toolchain/`: Python embebido + TinyCC). |
| GPU, GTE, SPU, CD-ROM, MDEC, timers | **Emulados por software** en el runtime de PSXRecomp (renderizado con OpenGL/SDL3), como en otros proyectos de recompilación estática. |
| BIOS | OpenBIOS (MIT, de PCSX-Redux), incluida. Se salta la animación de arranque. |

**Medido, no supuesto:** cada ejecución guarda en `psx_last_run_report.json` (`interp_hist`) cuánto código se interpretó.

- Con la caché de la v1.6.1, unos 20 minutos de juego en cuatro sesiones (partida nueva, la comisaría, puertas, escaleras, combate, inventario, baúl, guardado) llegaron como máximo al **0,03%–0,15%** del trabajo de la CPU emulada.
- Con la caché generada a partir del disco, una sesión en la comisaría con zombis y puertas midió **0,0061%**, con 17 instrucciones en el código de los enemigos. Lo que queda es código de la BIOS en RAM.

Las versiones anteriores (hasta la v1.6.0) se publicaron sin la caché de overlays ni el kit, así que el código de las salas se ejecutaba interpretado en los equipos de los jugadores. La v1.6.1 lo corrigió con una caché de las sesiones de juego del mantenedor. La v1.7.0 la sustituye por una caché generada a partir del disco.

No es una descompilación: el código del juego se traduce automáticamente, no se reescribe, y el hardware se emula. Por eso se mantienen rasgos del PS1 como el temblor de polígonos: la aritmética de punto fijo del GTE se reproduce fielmente.

## Nota legal

El ejecutable de la versión y los shards de overlay en `cache/` contienen el código máquina del juego traducido a x86-64. Debes poseer Resident Evil 3: Nemesis (SLUS-00923) y aportar tu propia imagen de disco. No se distribuyen imágenes de disco, audio, vídeo, texturas ni BIOS de Sony. Proyecto de fans sin fines comerciales; PSXRecomp está licenciado bajo PolyForm Noncommercial 1.0.0. Resident Evil y nombres relacionados son marcas de Capcom; PlayStation es marca de Sony Interactive Entertainment.

---

## Funciones

Leyenda: **Funciona** = confirmado · **Requiere pruebas** = implementado, no reverificado tras cambios recientes · **Problemas conocidos** = reportado como roto o parcial.

| Función | Estado |
|---|---|
| Asistente de instalación del launcher (extrae `SLUS_009.23` de tu `.cue`/`.bin`) | Funciona |
| Control clásico (tanque) | Funciona (por defecto) |
| Stick moderno relativo a la pantalla (`modern_controls`) | Requiere pruebas. El stick sigue los ejes fijos de cada cámara, como los controles alternativos de RE HD Remaster. En la v2.1.0, en un cambio de cámara la dirección actual continúa hasta soltar el stick o girarlo más de 45°; tras un corte a una cámara casi opuesta solo soltarlo la termina. Luego la dirección pasa a la nueva cámara suavemente (~0,2 s). La cruceta usa siempre los controles tanque. Desactivado por defecto |
| Puertas rápidas (`fast_doors`) | Funciona. Salta la animación de la puerta, y la sala siguiente carga a la velocidad máxima del PC tras la pantalla negra (la puerta de la sala de guardado pasó de ~3,8 s a ~1 s). La velocidad del juego no cambia |
| Saltar cinemáticas en tiempo real con Select (Esc/Tab en teclado) | Funciona (usa el salto del propio juego) |
| Saltar FMV | Funciona; algunos FMV solo se pueden saltar a partir de cierto punto, como en el juego original |
| Arranque rápido (animación de la BIOS omitida, huella del disco guardada) | Funciona |
| Guardado en memory card (`saves/*.mcd`, formato estándar de PS1) | Funciona |
| Audio | Funciona; la mezcla experimental (`spu_hq`) puede sonar saturada y viene desactivada |
| Trucos (opciones del launcher + F6/F7/F8) | Funcionaron en pruebas anteriores del mantenedor; no se han vuelto a probar desde la v1.6.0 |
| Resolución interna / supersampling, 16:9 estirado o 4:3 | Requiere pruebas |
| 16:9 Ampliado (`ws_layers`, opción Imagen del launcher) | Funciona en las sesiones del mantenedor. Nuevo en la v2.0.0: la escena 3D llena el 16:9 sin distorsión. Un paneo vertical suave sigue a Jill: queda quieto mientras ella está en la franja central, acelera y frena con suavidad y ya empieza encuadrado en cada cambio de cámara. Textos, avisos y la pausa mantienen su tamaño original. Las pantallas 2D (inventario, mapa, archivos, menús), los FMV y LIVE SELECTION quedan en 4:3. Algunas salas pierden un poco del techo o del suelo. Desactivado por defecto |
| Vibración | Requiere pruebas |
| Texturas con corrección de perspectiva | Requiere pruebas |
| Fuente nítida (opción del launcher) | Funciona. Nuevo en la v2.0.0: la primera partida con la opción extrae la fuente de tu propio juego, y el launcher la amplía 4x (EPX). Nada del juego va en el paquete |
| Corrección geométrica PGXP | Necesita prueba. Elimina el temblor. Nuevo en la v2.1.0: las juntas de los modelos (cara, nuca, axila) quedan cerradas: PGXP sigue la soldadura que el propio juego hace entre las partes del modelo, profundidad incluida, y la expansión de costuras de PSXRecomp cubre las partes sin vértice común. Desactivada por defecto |
| Velocidad del texto de introducción | Corregida según el mantenedor para la v2.0.0; requiere pruebas |
| Movimiento suave (fotogramas intermedios, opción avanzada del launcher) | Necesita prueba. El interpolador antiguo se eliminó (era placebo). Nuevo en la v2.1.0, experimental: usa el Smooth motion de PSXRecomp: dibuja la escena de nuevo entre dos fotogramas del juego, con los modelos a medio camino. La lógica y la velocidad del juego no cambian. Medido en un monitor de 144 Hz: ~72 imágenes distintas por segundo, frente a ~36 sin la opción. Funciona junto con la fuente nítida y el 16:9 Ampliado. Problema conocido: algunos artefactos en zombis. Desactivado por defecto |

Mandos antiguos que solo funcionan con DirectInput: activa "Mandos genéricos antiguos (DirectInput)" en el launcher. Viene desactivado porque en algunos PC retrasa el arranque varios segundos; los mandos de Xbox, PlayStation, Switch y la mayoría de los demás funcionan sin él.

Los reportes de errores son bienvenidos. Incluye el `psx_last_run_report.json` de la carpeta del juego y los pasos para reproducirlo.

---

## Inicio rápido

1. Descarga el zip más reciente en **Releases** y extráelo.
2. Ejecuta `Resident_Evil_3_Launcher.exe`.
3. Haz clic en **Instalar Juego...** y selecciona la imagen de tu disco de RE3 estadounidense (`.cue`/`.bin`, sectores en bruto de 2352 bytes). Los `.iso` de 2048 bytes se rechazan: no contienen el audio ni el vídeo del juego.
4. Elige los ajustes (se guardan solos; lo técnico está en **Opciones avanzadas...**) y haz clic en **JUGAR**.

## Controles

**Mando:** Stick izquierdo / D-Pad = movimiento (tanque por defecto; stick relativo a la pantalla en modo moderno) · A = acción · X = correr · Y = inventario · LB/RB = apuntar/disparar · Back = mapa · Start = pausa
**Teclado:** Flechas = mover · X = acción · Z = correr · A = inventario · Enter = start · Alt+Enter/F11 = pantalla completa · F6/F7/F8 = trucos

---

## Compilar desde el código fuente

Requisitos: Windows 10/11 x64, Clang (o MinGW-w64 GCC 12+), CMake 3.20+, Ninja, Python 3.11+ y tu propia imagen del disco (`.cue` + `.bin`).

```powershell
git config --global core.longpaths true   # o clona en una carpeta corta: el submódulo tiene rutas largas
git clone --recurse-submodules https://github.com/BrazilianHobbyist/Project-RE-Visited-RE3.git
cd Project-RE-Visited-RE3
.\build.ps1 -Regen -DiscPath "C:\ruta\a\Resident Evil 3 - Nemesis (USA, Canada).cue"   # juego
.\launcher\build.ps1    # launcher
```

Qué hace el build:

- Aplica `patches/runtime/*.patch` (los cambios de RE3, en orden) sobre el commit fijado de PSXRecomp.
- Con `-Regen`, recompila el C del juego a partir de tu disco en `generated/`. El código del juego nunca está en este repositorio.
- Aplica los dos `patches/re3_door_*.patch` al C generado.
- Compila con CMake y Ninja.

En los builds siguientes se puede omitir `-Regen`.

El build es reproducible. El mismo commit compilado con el mismo compilador produce un ejecutable y un launcher idénticos byte a byte. Se verificó con un clon limpio frente al build de desarrollo, usando Clang 22.1.8. `SOURCE_DATE_EPOCH` toma la fecha del commit y el enlazado no escribe marca de tiempo.

La caché nativa de overlays también se puede rehacer solo a partir de tu disco. Ver [aot/README.md](aot/README.md).

---

## Créditos

Ver [`CREDITS.md`](CREDITS.md).

* **PSXRecomp / RetroPortingToolkit**: framework de recompilación estática y runtime, por Matthew Stanley ("mstan") y colaboradores. Casi todo el trabajo de motor aquí es suyo.
* **PCSX-Redux / OpenBIOS**: BIOS de código abierto.
* **PGXP**: iCatButler, simias, Pete Bernert.
* **Capcom**: Resident Evil 3: Nemesis (1999).
