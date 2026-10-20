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
| Overlays (code the game loads from the CD for rooms, menus, inventory, etc.) | **Recompiled with the same recompiler** into native shards (`cache/`). The release ships shards for the areas played so far. Code not yet in the cache runs on a small interpreter the first time, is recorded, and is compiled in the background by the bundled toolchain (`overlay_toolchain/`: embedded Python + TinyCC), so it runs natively from the next visit on. |
| GPU, GTE, SPU, CD-ROM, MDEC, timers | **Emulated in software** by the PSXRecomp runtime (rendered with OpenGL/SDL2), as in other static recompilation projects. |
| BIOS | OpenBIOS (MIT, from PCSX-Redux), bundled. The boot animation is skipped. |

**Measured, not assumed:** in about 20 minutes of gameplay across four sessions (new game, the RPD, doors, stairs, combat, inventory, item box, saving), interpreted instructions were at most **0.03%–0.15%** of the emulated CPU's work; the rest ran as native code. Each run writes these numbers to `psx_last_run_report.json` (`interp_hist`).

Earlier releases (up to v1.6.0) shipped without the overlay cache and toolchain, so on players' machines the room code ran interpreted. That is fixed in v1.6.1.

This is not a decompilation: the game code is machine-translated, not rewritten, and the hardware is emulated. That is also why PS1 traits like polygon wobble remain: the GTE's fixed-point math is reproduced faithfully.

## Legal note

The release executable and the overlay shards in `cache/` contain the game's machine code translated to x86-64. You must own Resident Evil 3: Nemesis (SLUS-00923) and supply your own disc image. No disc images, audio, video, textures or Sony BIOS are distributed. This is a non-commercial fan project; PSXRecomp is licensed under PolyForm Noncommercial 1.0.0. Resident Evil and related names are trademarks of Capcom; PlayStation is a trademark of Sony Interactive Entertainment.

---

## Features

Status key: **Works** = confirmed working · **Needs testing** = implemented, not yet re-verified after recent changes · **Known issues** = reported broken or partial.

| Feature | Status |
|---|---|
| Launcher setup wizard (extracts `SLUS_009.23` from your `.cue`/`.bin`) | Works |
| Internal resolution / supersampling, 16:9 stretch or 4:3 | Needs testing |
| Classic (tank) controls | Works (default) |
| Modern screen-relative stick (`modern_controls`) | Known issues: works, but feels erratic in small rooms; off by default until refined |
| Skip door animations (`fast_doors`) | Works (silent transition with a short fade) |
| Skipping in-engine cutscenes with Select (Esc/Tab on keyboard) | Works (uses the game's own skip) |
| FMV skip | Works; some FMVs can only be skipped after a certain point, as in the original game |
| Fast startup (BIOS animation skipped, disc fingerprint cached) | Works |
| Memory card saves (`saves/*.mcd`, standard PS1 format) | Works |
| Audio | Works; the experimental mix (`spu_hq`) can sound clipped and is off by default |
| Cheats (launcher toggles + F6/F7/F8) | Works in the maintainer's earlier tests; not re-tested in v1.6.1 |
| Internal resolution / supersampling, 16:9 stretch or 4:3 | Needs testing |
| Rumble (SDL2) | Needs testing |
| Perspective-correct texturing | Needs testing |
| PGXP geometry correction | Known issues: stops wobble but causes artifacts on faces; off by default |
| Intro/story text speed | Known issues: text advances too fast |
| 60 FPS / frame interpolation | **Removed.** It never worked properly (it was placebo) |

Old DirectInput-only controllers: enable "Old generic controllers (DirectInput)" in the launcher. It is off by default because on some PCs it delays startup by several seconds; Xbox, PlayStation, Switch and most other pads work without it.

Bug reports are welcome. Please include `psx_last_run_report.json` from the game folder and the steps to reproduce.

---

## Quick start

1. Download the latest zip from **Releases** and extract it.
2. Run `Resident_Evil_3_Launcher.exe`.
3. Click **Install Game...** and select your RE3 USA disc image (`.cue`/`.bin`, raw 2352-byte sectors). 2048-byte `.iso` files are rejected: they lack the game's audio and video.
4. Configure, then click **PLAY RESIDENT EVIL 3**.

## Controls

**Gamepad:** Left stick / D-Pad = movement (tank by default; screen-relative stick in modern mode) · A = action · X = run · Y = inventory · LB/RB = aim/fire · Back = map · Start = pause
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
