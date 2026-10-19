# BRIEFING — 2026-10-18T00:35:00Z

## Mission
Perform a deep technical survey of the codebase regarding Requirements R3 (HD UI & Menus), R5 (Win32 Launcher & Configuration), and Build Pipelines (runtime & launcher).

## 🔒 My Identity
- Archetype: Explorer
- Roles: UI, Launcher & Configuration Explorer
- Working directory: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_survey_3\
- Original parent: 2974c4ea-67cd-482c-a523-a5380947c96d
- Milestone: Explorer Survey

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Produce survey_launcher_ui.md and handoff.md
- Comply with project-governance and global rules

## Current Parent
- Conversation ID: 2974c4ea-67cd-482c-a523-a5380947c96d
- Updated: 2026-10-18T00:35:00Z

## Investigation State
- **Explored paths**:
  - `ORIGINAL_REQUEST.md`
  - `launcher/main.cpp`, `launcher/build.ps1`, `launcher/resource.rc`
  - `build.ps1`, `CMakeLists.txt`, `game.toml`
  - `psxrecomp/recompiler/src/config_loader.h`, `config_loader.cpp`
  - `psxrecomp/runtime/src/main.cpp`, `gpu.c`, `gpu_gl_renderer.c`, `mod_texture_banks.c`
  - `patches/psxrecomp_runtime.patch`, `patches/re3_door_fade_speedup.patch`
- **Key findings**:
  - 2D UI screens (inventory, status, map, epilogues, save screen, clock) are rendered on PS1 using `POLY_FT4` quads, `SPRT` textured rectangles, and GP0 0xA0 `LoadImage` transfers to VRAM.
  - Runtime uses a dual-tier parsing system: `toml11` in `config_loader.cpp` and direct line-by-line in `load_re3_qol_settings()` (`main.cpp`).
  - Win32 launcher is in `launcher/main.cpp`, uses pure Win32 API + GDI+ dark theme, and serializes `settings.toml`.
  - Recommended UI implementation is a dedicated modal dialog `ShowTexturesDialog` mirroring `ShowCheatsDialog`, with folder enumeration via `FindFirstFileW` and Windows Explorer launch via `ShellExecuteW`.
  - Both `build.ps1` and `launcher/build.ps1` build cleanly with exit code 0.
- **Unexplored areas**: None for R3/R5 survey scope.

## Key Decisions Made
- Authored comprehensive survey report in `survey_launcher_ui.md`.
- Completed self-contained handoff in `handoff.md`.

## Artifact Index
- DISPATCH.md — Initial dispatch prompt
- progress.md — Liveness heartbeat and task tracker
- survey_launcher_ui.md — Comprehensive technical survey report
- handoff.md — 5-Component handoff report
