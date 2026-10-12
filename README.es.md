<p align="center">
  <img src="assets/banner.jpg" alt="Project RE: Visited - Resident Evil 3" width="100%">
</p>

# Project RE: Visited - Resident Evil 3

**Idiomas / Languages**: [English](README.md) | [Português (Brasil)](README.pt-br.md) | [**Español**](README.es.md)

---

**Port Nativo en C/C++ x86_64 y Recompilación Estática de Resident Evil 3: Nemesis (1999, PlayStation 1) para Windows**

---

## La Iniciativa Project RE: Visited

**Project RE: Visited** es una iniciativa independiente y de código abierto concebida para restaurar, modernizar y preservar históricamente la legendaria trilogía clásica de *survival horror* de PlayStation en PC:
* **Resident Evil 3: Nemesis** (Lanzamiento Inaugural)
* **Resident Evil 2** (Planificado)
* **Resident Evil** (Planificado)

El objetivo es ofrecer la mejor experiencia posible de estos clásicos en hardware contemporáneo, combinando fidelidad artística impecable con mejoras modernas de calidad de vida (QoL), controles analógicos 3D en 360°, tasas de fotogramas ultrafluidas y soporte de resoluciones de hasta 4K — sin depender de emuladores tradicionales o versiones antiguas y limitadas de PC.

---

## ¿Qué es la Recompilación Estática Clean-Room?

A diferencia de proyectos de emulación tradicional o de ingeniería inversa que dependen de filtraciones de código fuente, este proyecto fue desarrollado bajo una rigurosa metodología de **Recompilación Estática Limpia (*Clean Room Static Recompilation*)**:

```
+-----------------------------------------------------------------------------------+
|                             COPIA LEGÍTIMA DEL USUARIO                            |
|                    (CD-ROM original / Imagen ISO / BIN / CUE)                     |
+-----------------------------------------------------------------------------------+
                                          |
                         [Asistente de Instalación Nativo]
                   (Extracción directa del ejecutable SLUS_009.23)
                                          |
                                          v
+-----------------------+     +------------------------+     +----------------------+
|  Código de Máquina    |     |   Framework de         |     |  Código C Nativo     |
|  MIPS R3000A (PS1)    | --> |   Recompilación        | --> |  Moderno x86_64      |
|  (3.328 Funciones RE3)|     |   (PSXRecomp / Tool)   |     |  (Compilado en AOT)  |
+-----------------------+     +------------------------+     +----------------------+
                                                                        |
                                                                        v
+-----------------------------------------------------------------------------------+
|                    CAPA DE ABSTRACCIÓN DE HARDWARE (HAL ABIERTA)                  |
|      * Vídeo: OpenGL Moderno + PGXP (Sub-píxel y Perspectiva) + Shaders FXAA      |
|      * Audio: SPU HQ 48kHz Resampling en Coma Flotante                            |
|      * Entrada: XInput / DirectInput / SDL2 con Soporte Analógico 360° Real       |
|      * Sistema: Reimplementación de BIOS Abierta (OpenBIOS / HLE Callbacks)       |
+-----------------------------------------------------------------------------------+
```

### 1. Traducción Directa de Instrucciones de Máquina
El binario original compilado (`SLUS_009.23`) tiene sus instrucciones MIPS R3000A analizadas y traducidas estáticamente a código C intermedio de alto rendimiento. Cada rutina del juego se ejecuta como código nativo de máquina x86_64 en el procesador del ordenador, sin el impacto de rendimiento de decodificación en tiempo de ejecución (*JIT*) o ciclos de interpretación de un emulador.

### 2. Cero Líneas de Código Fuente Propietario de Capcom
Ningún código fuente filtrado, descompilación no autorizada de secretos industriales o material confidencial fue utilizado. El proyecto es 100% de desarrollo limpio (*clean-room*), tratando el binário original como una especificación estática de entrada.

### 3. Capa de Abstracción de Hardware Abierta (HAL)
Las llamadas al sistema, la GPU, el coprocesador geométrico GTE y el procesador de sonido SPU se traducen en tiempo de ejecución a APIs modernas y abiertas (OpenGL, SDL2, OpenBIOS).

### 4. Política Estrictamente Libre de Activos (*Zero-Asset Policy*)
**Este repositorio NO almacena, NO aloja y NO distribuye ningún archivo de juego, ROM, ISO, BIOS protegida, música, modelo o textura.** Todo el contenido protegido necesario para jugar es extraído directamente por el usuario a partir de su propia copia legítima del juego.

---

## Marco Legal y Exención de Responsabilidad

La ingeniería inversa con el fin estricto de **interoperabilidad de software y preservación digital** está ampliamente amparada por marcos legales y precedentes consolidados:

* **Estados Unidos**: *Digital Millennium Copyright Act* (DMCA), 17 U.S.C. § 1201(f) (*Interoperability Exception*), respaldado por las sentencias federales de los casos *Sega Enterprises Ltd. v. Accolade, Inc.* (977 F.2d 1510) y *Sony Computer Entertainment, Inc. v. Connectix Corp.* (203 F.3d 596).
* **Unión Europea**: Directiva 2009/24/CE del Parlamento Europeo y del Consejo, de 23 de abril de 2009, Artículo 6º (*Descompilación con el objetivo indispensable de lograr la interoperabilidad entre sistemas*).
* **Marcas Registradas**: *Resident Evil*, *Biohazard*, *Nemesis*, *Jill Valentine* son marcas comerciales de **Capcom Co., Ltd.**; *PlayStation* es marca comercial de **Sony Interactive Entertainment Inc.** Su uso es estrictamente nominativo y referencial (*nominative fair use*).
* **Términos Completos**: Consulte el documento legal detallado en [**`CREDITS.md`**](CREDITS.md).

---

## Características y Mejoras Implementadas

Todas las opciones enumeradas a continuación están **100% implementadas, auditadas y funcionales** en el proyecto:

| Característica | Descripción |
|---|---|
| **Asistente de Instalación Integrado** | Extracción nativa de sectores ISO 9660 Mode 2 Form 1. Detecta y extrae el ejecutable original `SLUS_009.23` de imágenes `.bin` / `.iso` en milisegundos sin herramientas externas. |
| **Controles Modernos 3D (360°)** | Control analógico real en el stick izquierdo con zona muerta calibrada (0.26), ángulos relativos a la cámara en tiempo real, carrera automática al inclinar más del 82% y rotación suave. La cruceta (D-Pad) mantiene 100% los controles clásicos de tanque. |
| **Fluidez en Altas Tasas (hasta 240 FPS)** | Interpolación poligonal temporal continua (`linear blend` + `flip-aware source`) que sincroniza los modelos 3D con la tasa de refresco del monitor (60Hz, 120Hz, 144Hz, 165Hz, 240Hz o Auto-Sync). |
| **Corrección Geométrica PGXP** | Elimina la oscilación poligonal (*polygon jitter/wobble*) clásica de PS1 mediante cálculo de vértices en coma flotante sub-píxel de alta precisión. |
| **Texturizado con Corrección de Perspectiva** | Mapeo de textura con corrección de perspectiva PGXP, eliminando distorsiones angulares en paredes y suelos. |
| **Panorámico 16:9 y Clásico 4:3** | Presentación moderna en pantalla completa sin bordes, pantalla completa exclusiva o ventana. Modelos 3D renderizados en proporción extendida con soporte visual a 16:9 o formato original 4:3. |
| **Resolución Interna 3D Escalable** | Renderizado interno de mallas poligonales en alta definición (1080p, 1440p y 4K Ultra HD) mediante supersampling (SSAA). |
| **Anti-Aliasing FXAA** | Filtro de postprocesamiento suave para bordes poligonales limpios y sin dientes de sierra perceptibles. |
| **Saltar FMVs / Cinemáticas (Auto-Skip)** | Salte instantáneamente presentaciones y cinemáticas en vídeo FMV (MDEC / XA) pulsando la tecla de acción o marcando la opción en el Launcher. |
| **Audio SPU de Alta Fidelidad (48 kHz)** | Resampling de audio ADPCM a 48.000 Hz en coma flotante mediante shadow engine de calibración dinámica. |
| **Launcher Gráfico Multilingüe** | Interfaz visual nativa en Win32 y GDI+ ultraligera con soporte dinámico a Español (ES), Inglés (EN) y Portugués (PT-BR). |
| **Memory Cards Preservadas** | Sistema de guardado estándar de PS1 grabado en archivos binarios reales `.mcd` (`saves/card1.mcd`), totalmente compatibles con emuladores reales y consolas físicas. |

---

## Aclaración de Alcance (Lo que este proyecto NO hace)

Para preservar la estabilidad técnica y la fidelidad histórica del juego:

1. **Sin Alteración Arbitraria de Memoria / Trainers Inestables**: No incluimos modificaciones forzadas de inventario o "item box hacks" directamente en la RAM, ya que corrompen el guardado en la máquina de estados de Resident Evil 3.
2. **Sin Desfiguración Artística con Paquetes de IA**: La fidelidad visual se mejora matemáticamente mediante PGXP y SSAA sobre las texturas auténticas de 1999, respetando el tono sombrío original de Capcom.
3. **Sin Cámara Libre 3D en Fondos Prerenderizados**: Los escenarios de *Resident Evil 3* son pinturas bidimensionales prerenderizadas en 4:3. El modo 16:9 adapta la presentación visual sin romper las colisiones invisibles del juego.

---

## Guía Rápida de Instalación

1. Ejecute **`Resident_Evil_3_Launcher.exe`**.
2. Si es la primera vez, haga clic en **`Instalar Jogo...`** (o *Setup Files*):
   * Seleccione su archivo de BIOS (`SCPH1001.BIN`).
   * Seleccione la imagen de su CD original (`.bin`, `.cue` o `.iso`).
   * Haga clic en **`Iniciar Instalación & Extracción`**. El instalador extraerá automáticamente `SLUS_009.23`.
3. Configure sus preferencias gráficas y de control.
4. ¡Haga clic en **`JOGAR RESIDENT EVIL 3`**!

---

## Controles

### Mando Xbox (XInput / Gamepad Moderno)
* **Stick Analógico Izquierdo**: Movimiento 360° analógico (Zona muerta 0.26 / Auto-run al inclinar al máximo)
* **Cruceta (D-Pad)**: Movimiento clásico tipo tanque (Girar / Avanzar / Retroceder)
* **A**: Acción / Confirmar / Disparar / Saltar Cinemática
* **B**: Cancelar / Correr
* **X**: Correr
* **Y**: Inventario
* **LB / LT**: Apuntar / Cambiar Objetivo
* **RB / RT**: Disparar / Apuntar Secundario
* **Back**: Mapa
* **Start**: Pausa

### Teclado
* **Flechas**: Movimiento
* **X**: Acción / Confirmar / Saltar FMV
* **S**: Cancelar / Correr
* **Z**: Correr
* **A**: Inventario
* **Enter**: Start
* **Alt + Enter** o **F11**: Alternar Pantalla Completa / Ventana

---

## ⌨️ Guardado (Memory Cards)

Los datos se guardan en el formato binario estándar de PS1 dentro de la carpeta `saves/`:
* `saves/card1.mcd` (Slot 1)
* `saves/card2.mcd` (Slot 2)

Compatible con DuckStation, Mednafen y hardware original de PS1.

---

## Créditos y Referencias

Consulte el documento completo [**`CREDITS.md`**](CREDITS.md) para atribuciones completas de Capcom, Sony, RetroPortingToolkit, Ghidra, DuckStation, PGXP, SDL2 y demás colaboradores.
