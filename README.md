<p align="center">
  <img src="assets/banner.jpg" alt="Project RE: Visited - Resident Evil 3" width="100%">
</p>

# Project RE: Visited - Resident Evil 3

**Languages / Idiomas**: [**English**](README.md) | [Português (Brasil)](README.pt-br.md) | [Español](README.es.md)

---

**Experimental Windows port of Resident Evil 3: Nemesis (PS1, SLUS-00923), built on the [PSXRecomp](https://github.com/RetroPortingToolKit/psxrecomp) static recompilation framework.**

> **Status: work in progress / experimental.** Expect bugs. Earlier release notes and this README overstated what works; this version describes the project as it actually is.

---

## How it actually works

| Part | How it runs |
|---|---|
| Main executable (`SLUS_009.23`, 3,328 functions) | **Statically recompiled** MIPS → C → x86-64. Runs as native code. |
| Overlays (per-room/stage code the game loads from the CD at `0x80165000`) | **Interpreted** by default. They are only compiled to native code at runtime if a C compiler (`gcc`/`clang`) is on your `PATH`. Even then, most overlay calls currently still go through the interpreter. |
| GPU, GTE, SPU, CD-ROM, MDEC, timers | **Emulated in software** by the PSXRecomp runtime (rendered with OpenGL/SDL2). |
| BIOS | OpenBIOS (MIT, from PCSX-Redux), bundled. |

So this is a hybrid: the game's main code is recompiled, but a large share of what runs during gameplay is interpreted, and the hardware is emulated. It is not a "native PC port" in the sense of decompiled, rewritten game code. That is also why PS1 traits like polygon wobble remain: the GTE's fixed-point math is reproduced faithfully.

## Legal note

The release executable contains the game's machine code translated to x86-64. You must own Resident Evil 3: Nemesis (SLUS-00923) and supply your own disc image. No disc images, audio, video, textures or Sony BIOS are distributed. This is a non-commercial fan project; PSXRecomp is licensed under PolyForm Noncommercial 1.0.0. Resident Evil and related names are trademarks of Capcom; PlayStation is a trademark of Sony Interactive Entertainment.

---

## Features

Status key: **Works** = confirmed working · **Needs testing** = implemented, not yet re-verified after recent changes · **Known issues** = reported broken or partial.

| Feature | Status |
|---|---|
| Launcher setup wizard (extracts `SLUS_009.23` from your `.bin`/`.cue`/`.iso`) | Works |
| Internal resolution / supersampling, 16:9 stretch or 4:3 | Needs testing |
| Modern analog controls with auto-run (`modern_controls`) | Known issues: was broken in v1.4.x, restored in v1.5.1, needs re-testing |
| Fast doors (`fast_doors`) | Known issues: skipping the cutscene after a door can freeze the game |
| FMV skip | Needs testing |
| Rumble (SDL2) | Needs testing |
| Cheats (launcher toggles + F6/F7/F8) | Needs testing |
| PGXP geometry correction | Known issues: no visible effect / deforms characters; not recommended |
| Perspective-correct texturing | Needs testing |
| Audio | Known issues: crackling on room transitions |
| Intro/story text speed | Known issues: text advances too fast |
| 60 FPS / frame interpolation | **Removed.** It never worked properly (it was placebo) |
| Memory card saves (`saves/*.mcd`, standard PS1 format) | Needs testing |

Bug reports are welcome. Please include `psx_last_run_report.json` from the game folder and the steps to reproduce.

---

## Quick start

1. Download the latest zip from **Releases** and extract it.
2. Run `Resident_Evil_3_Launcher.exe`.
3. Click **Setup Files...** and select your RE3 disc image (`.bin`, `.cue` or `.iso`).
4. Configure, then click **PLAY RESIDENT EVIL 3**.

## Controls

**Gamepad:** Left stick = movement (modern mode) · D-Pad = classic tank controls · A = action · X = run · Y = inventory · LB/RB = aim/fire · Back = map · Start = pause
**Keyboard:** Arrows = move · X = action · Z = run · A = inventory · Enter = start · Alt+Enter/F11 = fullscreen · F6/F7/F8 = cheats

---

## Building from source

Requirements: Windows 10/11 x64, MinGW-w64 GCC 12+ or Clang, CMake 3.20+, Ninja, Python 3.8+, and your own copy of `SLUS_009.23`.

```powershell
git clone --recurse-submodules https://github.com/BrazilianHobbyist/Project-RE-Visited-RE3.git
.\build.ps1             # game
.\launcher\build.ps1    # launcher
```

Runtime changes on top of upstream PSXRecomp are in `patches/`.

---

## Credits

See [`CREDITS.md`](CREDITS.md).

* **PSXRecomp / RetroPortingToolkit**: static recompilation framework and runtime, by Matthew Stanley ("mstan") and contributors. Almost all of the engine work here is theirs.
* **PCSX-Redux / OpenBIOS**: open-source BIOS.
* **PGXP**: iCatButler, simias, Pete Bernert.
* **Capcom**: Resident Evil 3: Nemesis (1999).
