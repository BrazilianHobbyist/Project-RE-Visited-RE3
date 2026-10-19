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
| Overlays (código de cada sala/etapa cargado desde el CD en `0x80165000`) | **Interpretados** por defecto. Solo se compilan a nativo en tiempo de ejecución si hay un compilador C (`gcc`/`clang`) en el `PATH`. Aun así, la mayoría de las llamadas de overlay siguen pasando por el intérprete. |
| GPU, GTE, SPU, CD-ROM, MDEC, timers | **Emulados por software** en el runtime de PSXRecomp (renderizado con OpenGL/SDL2). |
| BIOS | OpenBIOS (MIT, de PCSX-Redux), incluida. |

Es un híbrido: el código principal del juego está recompilado, pero gran parte de lo que se ejecuta durante el juego es interpretado, y el hardware es emulado. No es un "port nativo" en el sentido de código descompilado y reescrito. Por eso se mantienen rasgos del PS1 como el temblor de polígonos: la aritmética de punto fijo del GTE se reproduce fielmente.

## Nota legal

El ejecutable de la versión contiene el código máquina del juego traducido a x86-64. Debes poseer Resident Evil 3: Nemesis (SLUS-00923) y aportar tu propia imagen de disco. No se distribuyen imágenes de disco, audio, vídeo, texturas ni BIOS de Sony. Proyecto de fans sin fines comerciales; PSXRecomp está licenciado bajo PolyForm Noncommercial 1.0.0. Resident Evil y nombres relacionados son marcas de Capcom; PlayStation es marca de Sony Interactive Entertainment.

---

## Funciones

Leyenda: **Funciona** = confirmado · **Requiere pruebas** = implementado, no reverificado tras cambios recientes · **Problemas conocidos** = reportado como roto o parcial.

| Función | Estado |
|---|---|
| Asistente de instalación del launcher (extrae `SLUS_009.23` de tu `.bin`/`.cue`/`.iso`) | Funciona |
| Resolución interna / supersampling, 16:9 estirado o 4:3 | Requiere pruebas |
| Controles analógicos modernos con auto-correr (`modern_controls`) | Problemas conocidos: roto en v1.4.x, restaurado en v1.5.1, requiere nuevas pruebas |
| Puertas rápidas (`fast_doors`) | Problemas conocidos: saltar la cinemática tras una puerta puede congelar el juego |
| Saltar FMV | Requiere pruebas |
| Vibración (SDL2) | Requiere pruebas |
| Trucos (opciones del launcher + F6/F7/F8) | Requiere pruebas |
| Corrección geométrica PGXP | Problemas conocidos: sin efecto visible / deforma personajes; no recomendado |
| Texturas con corrección de perspectiva | Requiere pruebas |
| Audio | Problemas conocidos: chasquidos en transiciones de sala |
| Velocidad del texto de introducción | Problemas conocidos: el texto avanza demasiado rápido |
| 60 FPS / interpolación de fotogramas | **Eliminado.** Nunca funcionó realmente (era placebo) |
| Guardado en memory card (`saves/*.mcd`, formato estándar de PS1) | Requiere pruebas |

Los reportes de errores son bienvenidos. Incluye el `psx_last_run_report.json` de la carpeta del juego y los pasos para reproducirlo.

---

## Inicio rápido

1. Descarga el zip más reciente en **Releases** y extráelo.
2. Ejecuta `Resident_Evil_3_Launcher.exe`.
3. Haz clic en **Instalar Juego** y selecciona la imagen de tu disco de RE3 (`.bin`, `.cue` o `.iso`).
4. Configura y haz clic en **JUGAR RESIDENT EVIL 3**.

## Controles

**Mando:** Stick izquierdo = movimiento (modo moderno) · D-Pad = control tanque clásico · A = acción · X = correr · Y = inventario · LB/RB = apuntar/disparar · Back = mapa · Start = pausa
**Teclado:** Flechas = mover · X = acción · Z = correr · A = inventario · Enter = start · Alt+Enter/F11 = pantalla completa · F6/F7/F8 = trucos

---

## Compilar desde el código fuente

Requisitos: Windows 10/11 x64, MinGW-w64 GCC 12+ o Clang, CMake 3.20+, Ninja, Python 3.8+ y tu propia copia de `SLUS_009.23`.

```powershell
git clone --recurse-submodules https://github.com/BrazilianHobbyist/Project-RE-Visited-RE3.git
.\build.ps1             # juego
.\launcher\build.ps1    # launcher
```

Los cambios del runtime respecto a PSXRecomp original están en `patches/`.

---

## Créditos

Ver [`CREDITS.md`](CREDITS.md).

* **PSXRecomp / RetroPortingToolkit**: framework de recompilación estática y runtime, por Matthew Stanley ("mstan") y colaboradores. Casi todo el trabajo de motor aquí es suyo.
* **PCSX-Redux / OpenBIOS**: BIOS de código abierto.
* **PGXP**: iCatButler, simias, Pete Bernert.
* **Capcom**: Resident Evil 3: Nemesis (1999).
