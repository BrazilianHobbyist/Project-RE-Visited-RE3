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
| Overlays (code the game loads from the CD for menus and enemies) | **Recompiled with the same recompiler** into native shards (`cache/`). Since v1.7.0 the cache is built **from the original disc**, not from play sessions: all 17 menu overlays and all 21 enemy code modules, 59 images in total ([aot/README.md](aot/README.md)). Anything still missing runs on a small interpreter the first time and is compiled in the background by the bundled toolchain (`overlay_toolchain/`: embedded Python + TinyCC). |
| GPU, GTE, SPU, CD-ROM, MDEC, timers | **Emulated in software** by the PSXRecomp runtime (rendered with OpenGL/SDL3), as in other static recompilation projects. |
| BIOS | OpenBIOS (MIT, from PCSX-Redux), bundled. The boot animation is skipped. |

**Measured, not assumed:** each run writes how much code was interpreted to `psx_last_run_report.json` (`interp_hist`).

- With the v1.6.1 cache, about 20 minutes of play across four sessions (new game, the RPD, doors, stairs, combat, inventory, item box, saving) peaked at **0.03%–0.15%** of the emulated CPU's work.
- With the disc-built cache, a session in the RPD with zombies and doors measured **0.0061%**, and 17 instructions in the enemy code. What remains is BIOS code in RAM.

Earlier releases (up to v1.6.0) shipped without the overlay cache and toolchain, so the room code ran interpreted on players' machines. v1.6.1 fixed that with a cache from the maintainer's play sessions. v1.7.0 replaces it with one built from the disc.

This is not a decompilation: the game code is machine-translated, not rewritten, and the hardware is emulated. That is also why PS1 traits like polygon wobble remain: the GTE's fixed-point math is reproduced faithfully.

## Legal note

The release executable and the overlay shards in `cache/` contain the game's machine code translated to x86-64. You must own Resident Evil 3: Nemesis (SLUS-00923) and supply your own disc image. No disc images, audio, video, textures or Sony BIOS are distributed. This is a non-commercial fan project; PSXRecomp is licensed under PolyForm Noncommercial 1.0.0. Resident Evil and related names are trademarks of Capcom; PlayStation is a trademark of Sony Interactive Entertainment.

---

## Features

Status key: **Works** = confirmed working · **Needs testing** = implemented, not yet re-verified after recent changes · **Known issues** = reported broken or partial.

| Feature | Status |
|---|---|
| Launcher setup wizard (extracts `SLUS_009.23` from your `.cue`/`.bin`) | Works |
| Classic (tank) controls | Works (default) |
| Modern screen-relative stick (`modern_controls`) | Needs testing. Since v2.0.0 the stick follows each camera's fixed screen axes, and the heading kept through a camera cut ends when you release the stick (before, direction drifted and could flip near the camera). Off by default |
| Fast doors (`fast_doors`) | Works. Skips the door animation, and the next room loads at full host speed behind the black screen (the save-room door went from ~3.8 s to ~1 s). Gameplay speed is unchanged |
| Skipping in-engine cutscenes with Select (Esc/Tab on keyboard) | Works (uses the game's own skip) |
| FMV skip | Works; some FMVs can only be skipped after a certain point, as in the original game |
| Fast startup (BIOS animation skipped, disc fingerprint cached) | Works |
| Memory card saves (`saves/*.mcd`, standard PS1 format) | Works |
| Audio | Works; the experimental mix (`spu_hq`) can sound clipped and is off by default |
| Cheats (launcher toggles + F6/F7/F8) | Works in the maintainer's earlier tests; not re-tested since v1.6.0 |
| Internal resolution / supersampling, 16:9 stretch or 4:3 | Needs testing |
| 16:9 Zoomed (`ws_layers`, launcher Picture option) | Works in the maintainer's play sessions. New in v2.0.0: the 3D scene fills 16:9 without distortion. A smooth vertical pan follows Jill: it holds still while she stays in the middle band, eases in and out, and starts already framed on each camera cut. Text, prompts and the pause overlay stay at their original size. Full 2D screens (inventory, map, files, menus), FMV and LIVE SELECTION stay 4:3. Some rooms lose a little of the ceiling or floor. Off by default |
| Rumble | Needs testing |
| Perspective-correct texturing | Needs testing |
| Sharp font (launcher option) | Works. New in v2.0.0: the first session with the option on extracts the font from your own game, and the launcher then upscales it 4x (EPX). Nothing from the game is shipped |
| PGXP geometry correction | Known issues: stops wobble. The black cracks on faces are much smaller since v1.7.0, because seam vertices are welded, but small artifacts remain. Off by default |
| Intro/story text speed | Reported fixed by the maintainer for v2.0.0; needs testing |
| 60 FPS / frame interpolation | **Removed.** It never worked properly (it was placebo) |

Old DirectInput-only controllers: enable "Old generic controllers (DirectInput)" in the launcher. It is off by default because on some PCs it delays startup by several seconds; Xbox, PlayStation, Switch and most other pads work without it.

Bug reports are welcome. Please include `psx_last_run_report.json` from the game folder and the steps to reproduce.

---

## Quick start

1. Download the latest zip from **Releases** and extract it.
2. Run `Resident_Evil_3_Launcher.exe`.
3. Click **Install Game...** and select your RE3 USA disc image (`.cue`/`.bin`, raw 2352-byte sectors). 2048-byte `.iso` files are rejected: they lack the game's audio and video.
4. Pick your settings (they save on their own; technical ones are under **Advanced options...**), then click **PLAY**.

## Controls

**Gamepad:** Left stick / D-Pad = movement (tank by default; screen-relative stick in modern mode) · A = action · X = run · Y = inventory · LB/RB = aim/fire · Back = map · Start = pause
**Keyboard:** Arrows = move · X = action · Z = run · A = inventory · Enter = start · Alt+Enter/F11 = fullscreen · F6/F7/F8 = cheats

---

## Building from source

Requirements: Windows 10/11 x64, Clang (or MinGW-w64 GCC 12+), CMake 3.20+, Ninja, Python 3.11+, and your own disc image (`.cue` + `.bin`).

```powershell
git config --global core.longpaths true   # or clone into a short folder: the submodule has deep paths
git clone --recurse-submodules https://github.com/BrazilianHobbyist/Project-RE-Visited-RE3.git
cd Project-RE-Visited-RE3
.\build.ps1 -Regen -DiscPath "C:\path\to\Resident Evil 3 - Nemesis (USA, Canada).cue"   # game
.\launcher\build.ps1    # launcher
```

What the build does:

- Applies `patches/runtime/*.patch` (the RE3 changes, in order) on top of the pinned PSXRecomp commit.
- `-Regen` recompiles the game's C from your disc into `generated/`. Game code is never in this repository.
- Applies the two `patches/re3_door_*.patch` to the generated C.
- Builds with CMake and Ninja.

Later builds can drop `-Regen`.

The build is reproducible. The same commit built with the same compiler produces a byte-identical executable and launcher. This was verified with a fresh clone against the development build, using Clang 22.1.8. `SOURCE_DATE_EPOCH` is set to the commit time, and no link timestamp is written.

The native overlay cache can also be rebuilt from your disc alone. See [aot/README.md](aot/README.md).

---

## Credits

See [`CREDITS.md`](CREDITS.md).

* **PSXRecomp / RetroPortingToolkit**: static recompilation framework and runtime, by Matthew Stanley ("mstan") and contributors. Almost all of the engine work here is theirs.
* **PCSX-Redux / OpenBIOS**: open-source BIOS.
* **PGXP**: iCatButler, simias, Pete Bernert.
* **Capcom**: Resident Evil 3: Nemesis (1999).
